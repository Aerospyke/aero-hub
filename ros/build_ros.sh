#!/usr/bin/env bash
# Rebuild ah_msgs / ah_core / ah_yolo via colcon.
# Used by CMake target "build_ros" and safe to run by hand.
#
#   ./ros/build_ros.sh
#   cmake --build build/Debug --target build_ros
#
set -euo pipefail

_AH_ROS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "${_AH_ROS_DIR}"

# --- Ensure colcon is on PATH (CMake/CLion often lack conda activation) ---
_find_colcon() {
  if command -v colcon >/dev/null 2>&1; then
    command -v colcon
    return 0
  fi
  if [[ -n "${CONDA_PREFIX:-}" && -x "${CONDA_PREFIX}/bin/colcon" ]]; then
    echo "${CONDA_PREFIX}/bin/colcon"
    return 0
  fi
  local candidates=(
    "${HOME}/miniconda3/envs/ros_env/bin/colcon"
    "${HOME}/mambaforge/envs/ros_env/bin/colcon"
    "${HOME}/anaconda3/envs/ros_env/bin/colcon"
  )
  local c
  for c in "${candidates[@]}"; do
    if [[ -x "${c}" ]]; then
      echo "${c}"
      return 0
    fi
  done
  return 1
}

# Source conda + activate ros_env if colcon still missing
if ! _find_colcon >/dev/null 2>&1; then
  for _conda_sh in \
    "${HOME}/miniconda3/etc/profile.d/conda.sh" \
    "${HOME}/mambaforge/etc/profile.d/conda.sh" \
    "${HOME}/anaconda3/etc/profile.d/conda.sh"
  do
    if [[ -f "${_conda_sh}" ]]; then
      # shellcheck disable=SC1090
      source "${_conda_sh}"
      if conda env list 2>/dev/null | grep -qE '^ros_env\s'; then
        conda activate ros_env
      fi
      break
    fi
  done
fi

if ! COLCON_BIN="$(_find_colcon)"; then
  echo "error: colcon not found." >&2
  echo "  Activate RoboStack first, e.g.:  conda activate ros_env" >&2
  echo "  Then re-run, or ensure ~/miniconda3/envs/ros_env/bin/colcon exists." >&2
  exit 127
fi

# Put env bin first so python/cmake used by colcon match the package.
_COLCON_DIR="$(cd "$(dirname "${COLCON_BIN}")" && pwd)"
export PATH="${_COLCON_DIR}:${PATH}"
if [[ -z "${CONDA_PREFIX:-}" || ! -d "${CONDA_PREFIX}" ]]; then
  # e.g. .../envs/ros_env/bin → .../envs/ros_env
  export CONDA_PREFIX="$(cd "${_COLCON_DIR}/.." && pwd)"
fi

# RoboStack / ament: packages resolve via prefix paths (CLion often omits these).
export AMENT_PREFIX_PATH="${CONDA_PREFIX}${AMENT_PREFIX_PATH:+:${AMENT_PREFIX_PATH}}"
export CMAKE_PREFIX_PATH="${CONDA_PREFIX}${CMAKE_PREFIX_PATH:+:${CMAKE_PREFIX_PATH}}"
export PYTHONPATH="${CONDA_PREFIX}/lib/python3.12/site-packages${PYTHONPATH:+:${PYTHONPATH}}"
export RMW_IMPLEMENTATION="${RMW_IMPLEMENTATION:-rmw_fastrtps_cpp}"
export ROS_DOMAIN_ID="${ROS_DOMAIN_ID:-42}"

echo "[build_ros] colcon=${COLCON_BIN}"
echo "[build_ros] cwd=${_AH_ROS_DIR}"
echo "[build_ros] CONDA_PREFIX=${CONDA_PREFIX:-}"

"${COLCON_BIN}" build \
  --packages-select ah_msgs ah_core ah_yolo \
  --cmake-args -DCMAKE_BUILD_TYPE=Release \
  "$@"
