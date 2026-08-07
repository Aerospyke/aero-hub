#!/usr/bin/env bash
# Copy an existing AeroHub.app and stage ROS shared libraries into Frameworks.
# Does NOT run macdeployqt — iterate on ROS bundling without a full Qt redeploy.
#
# Usage (from aero-hub/):
#   ./scripts/bundle_ros_into_app.sh
#   ./scripts/bundle_ros_into_app.sh \
#       --src run/AeroHub.app \
#       --dst run/AeroHub-standalone.app
#
# Prerequisites:
#   - A working lab app already installed (cmake --install …) at --src
#   - CONDA_PREFIX or default ~/miniconda3/envs/ros_env with ROS dylibs
#   - ros/install (ah_msgs, etc.)
#
set -eo pipefail

_AH_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
_SRC="${_AH_ROOT}/run/AeroHub.app"
_DST="${_AH_ROOT}/run/AeroHub-standalone.app"
_CONDA_LIB="${CONDA_PREFIX:+${CONDA_PREFIX}/lib}"
_CONDA_LIB="${_CONDA_LIB:-${HOME}/miniconda3/envs/ros_env/lib}"
_ROS_INSTALL="${_AH_ROOT}/ros/install"
_APP_NAME="AeroHub"

while [[ $# -gt 0 ]]; do
  case "$1" in
    --src) _SRC="$(cd "$(dirname "$2")" && pwd)/$(basename "$2")"; shift 2 ;;
    --dst) _DST="$(cd "$(dirname "$2")" && pwd)/$(basename "$2")"; shift 2 ;;
    --conda-lib) _CONDA_LIB="$2"; shift 2 ;;
    --ros-install) _ROS_INSTALL="$2"; shift 2 ;;
    -h|--help)
      sed -n '2,20p' "$0" | sed 's/^# \{0,1\}//'
      exit 0
      ;;
    *)
      echo "error: unknown arg: $1" >&2
      exit 2
      ;;
  esac
done

_EXE="${_DST}/Contents/MacOS/${_APP_NAME}"
_FW="${_DST}/Contents/Frameworks"

if [[ ! -d "${_SRC}/Contents/MacOS" ]]; then
  echo "error: source app missing or incomplete: ${_SRC}" >&2
  echo "  First: cmake --build … --target AeroHub && cmake --install …" >&2
  exit 1
fi
if [[ ! -d "${_CONDA_LIB}" ]]; then
  echo "error: conda ROS lib dir not found: ${_CONDA_LIB}" >&2
  echo "  Activate ros_env or pass --conda-lib" >&2
  exit 1
fi

echo "[bundle_ros] src=${_SRC}"
echo "[bundle_ros] dst=${_DST}"
echo "[bundle_ros] conda_lib=${_CONDA_LIB}"
echo "[bundle_ros] ros_install=${_ROS_INSTALL}"

# Fresh copy so we never mutate the lab install app.
rm -rf "${_DST}"
ditto "${_SRC}" "${_DST}"
mkdir -p "${_FW}"

# Search roots for resolving @rpath basenames.
_SEARCH_DIRS=("${_CONDA_LIB}")
if [[ -d "${_ROS_INSTALL}" ]]; then
  while IFS= read -r -d '' _libdir; do
    _SEARCH_DIRS+=("${_libdir}")
  done < <(find "${_ROS_INSTALL}" -type d -name lib -print0 2>/dev/null)
fi

_find_lib() {
  local name=$1
  local d
  for d in "${_SEARCH_DIRS[@]}"; do
    if [[ -f "${d}/${name}" ]]; then
      echo "${d}/${name}"
      return 0
    fi
  done
  return 1
}

_stage_lib() {
  local src=$1
  local name
  name="$(basename "${src}")"
  if [[ ! -f "${_FW}/${name}" ]]; then
    cp -f "${src}" "${_FW}/${name}"
    echo "[bundle_ros] + ${name}"
  fi
  # id relative to Frameworks (main binary uses @loader_path/../Frameworks/…)
  install_name_tool -id "@loader_path/../Frameworks/${name}" "${_FW}/${name}" 2>/dev/null || true
}

# Seed: RMW + common typesupport plugins (dlopen, not always link-time) + ah_msgs
# + main-binary @rpath gaps. Also re-scan every Frameworks dylib already present
# from macdeployqt (Qt/ROS link-time set) so their missing @rpath deps get filled.
_SEED=()
for _pat in \
  librmw_fastrtps*.dylib \
  librmw_dds_common*.dylib \
  librosidl_typesupport_fastrtps*.dylib \
  librosidl_dynamic_typesupport_fastrtps*.dylib \
  libfastrtps*.dylib \
  libfastcdr*.dylib \
  libfoonathan_memory*.dylib
do
  while IFS= read -r -d '' _f; do
    _SEED+=("${_f}")
  done < <(find "${_CONDA_LIB}" -maxdepth 1 -name "${_pat}" -print0 2>/dev/null)
done
if [[ -d "${_ROS_INSTALL}/ah_msgs/lib" ]]; then
  while IFS= read -r -d '' _f; do
    _SEED+=("${_f}")
  done < <(find "${_ROS_INSTALL}/ah_msgs/lib" -name 'libah_msgs*.dylib' -print0 2>/dev/null)
fi
while IFS= read -r _dep; do
  _base="${_dep#@rpath/}"
  if [[ -f "${_FW}/${_base}" ]]; then
    continue
  fi
  if _path="$(_find_lib "${_base}")"; then
    _SEED+=("${_path}")
  fi
done < <(otool -L "${_EXE}" 2>/dev/null | awk '/@rpath\//{print $1}' | sort -u)

# Multi-pass: stage queue, then scan *all* Frameworks dylibs for missing @rpath.
_QUEUE=("${_SEED[@]}")
for _pass in $(seq 1 25); do
  _next=()
  for _src in "${_QUEUE[@]+"${_QUEUE[@]}"}"; do
    [[ -f "${_src}" ]] || continue
    _stage_lib "${_src}"
  done
  _QUEUE=()
  for _dst in "${_FW}"/*.dylib; do
    [[ -f "${_dst}" ]] || continue
    while IFS= read -r _dep; do
      _base="${_dep#@rpath/}"
      [[ -n "${_base}" ]] || continue
      if [[ -f "${_FW}/${_base}" ]]; then
        continue
      fi
      if _path="$(_find_lib "${_base}")"; then
        _QUEUE+=("${_path}")
      fi
    done < <(otool -L "${_dst}" 2>/dev/null | awk '/@rpath\//{print $1}' | sort -u)
  done
  # also main binary
  while IFS= read -r _dep; do
    _base="${_dep#@rpath/}"
    if [[ -f "${_FW}/${_base}" ]]; then
      continue
    fi
    if _path="$(_find_lib "${_base}")"; then
      _QUEUE+=("${_path}")
    fi
  done < <(otool -L "${_EXE}" 2>/dev/null | awk '/@rpath\//{print $1}' | sort -u)

  # uniq queue
  if [[ ${#_QUEUE[@]} -eq 0 ]]; then
    break
  fi
  # de-dupe
  _uniq=()
  for _p in "${_QUEUE[@]}"; do
    _n="$(basename "${_p}")"
    if [[ -f "${_FW}/${_n}" ]]; then
      continue
    fi
    _uniq+=("${_p}")
  done
  # naive unique by path
  _QUEUE=()
  while IFS= read -r _line; do
    [[ -n "${_line}" ]] && _QUEUE+=("${_line}")
  done < <(printf '%s\n' "${_uniq[@]+"${_uniq[@]}"}" | sort -u)
  [[ ${#_QUEUE[@]} -eq 0 ]] && break
done

# Rewrite @rpath → @loader_path among Frameworks dylibs
for _dst in "${_FW}"/*.dylib; do
  [[ -f "${_dst}" ]] || continue
  while IFS= read -r _dep; do
    _base="${_dep#@rpath/}"
    if [[ -f "${_FW}/${_base}" ]]; then
      install_name_tool -change "${_dep}" "@loader_path/${_base}" "${_dst}" 2>/dev/null || true
    fi
  done < <(otool -L "${_dst}" 2>/dev/null | awk '/@rpath\//{print $1}' | sort -u)
done

# Main binary: point remaining @rpath at Frameworks
while IFS= read -r _dep; do
  _base="${_dep#@rpath/}"
  if [[ -f "${_FW}/${_base}" ]]; then
    install_name_tool -change "${_dep}" "@loader_path/../Frameworks/${_base}" "${_EXE}" 2>/dev/null || true
  fi
done < <(otool -L "${_EXE}" 2>/dev/null | awk '/@rpath\//{print $1}' | sort -u)

# Drop absolute lab rpaths from the *copy* so cold start exercises Frameworks only
# (source app may still have CONDA / ros/install LC_RPATH from lab install).
while IFS= read -r _rpath; do
  case "${_rpath}" in
    @loader_path/*|@executable_path/*) ;;
    /*)
      echo "[bundle_ros] remove absolute rpath: ${_rpath}"
      install_name_tool -delete_rpath "${_rpath}" "${_EXE}" 2>/dev/null || true
      ;;
  esac
done < <(otool -l "${_EXE}" 2>/dev/null | awk '/cmd LC_RPATH/{getline; getline; sub(/^ *path /,""); sub(/ \(offset.*$/,""); print}')

# Ensure rpath to Frameworks for dlopen basenames that resolve via LC_RPATH
install_name_tool -add_rpath "@loader_path/../Frameworks" "${_EXE}" 2>/dev/null || true
install_name_tool -add_rpath "@executable_path/../Frameworks" "${_EXE}" 2>/dev/null || true

# Report unresolved @rpath still pointing outside Frameworks
_MISSING=0
while IFS= read -r _dep; do
  _base="${_dep#@rpath/}"
  if [[ ! -f "${_FW}/${_base}" ]]; then
    echo "[bundle_ros] warning: still unresolved on main binary: ${_dep}" >&2
    _MISSING=1
  fi
done < <(otool -L "${_EXE}" 2>/dev/null | awk '/@rpath\//{print $1}' | sort -u)

for _dst in "${_FW}"/*.dylib; do
  [[ -f "${_dst}" ]] || continue
  while IFS= read -r _dep; do
    _base="${_dep#@rpath/}"
    if [[ ! -f "${_FW}/${_base}" ]]; then
      echo "[bundle_ros] warning: $(basename "${_dst}") still needs: ${_dep}" >&2
      _MISSING=1
    fi
  done < <(otool -L "${_dst}" 2>/dev/null | awk '/@rpath\//{print $1}' | sort -u)
done

echo "[bundle_ros] codesigning…"
codesign --force --deep --sign - "${_DST}"

_count="$(find "${_FW}" -name '*.dylib' | wc -l | tr -d ' ')"
echo "[bundle_ros] done: ${_DST}  (${_count} dylibs in Frameworks)"
if [[ "${_MISSING}" -ne 0 ]]; then
  echo "[bundle_ros] finished with unresolved warnings (app may still crash on missing plugins)." >&2
  exit 3
fi
echo "[bundle_ros] test:  ${_DST}/Contents/MacOS/${_APP_NAME}"
echo "[bundle_ros] note:  ament share/ is not copied; pure dylib bundling may still need AMENT_PREFIX_PATH for some ROS features."
