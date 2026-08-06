# Source this in every terminal that talks to the AeroHub ROS graph (bash or zsh).
#
#   cd …/aero-hub          # CWD must contain aerohub_settings.ini
#   source ros/init_ah_ros_in_terminal.sh
#
# This only:
#   1) loads the colcon overlay (install/setup.*)
#   2) exports ROS_DOMAIN_ID, RMW_IMPLEMENTATION, AERO_HUB_YOLO_MODELS from the INI
#      via AhCommon (ah_settings_shell_exports) — no hard-coded defaults here
#
# Namespace is NOT exported: each node applies it from settings.
#
# Build AhCommon first if the helper is missing:
#   cmake --build …/build/Debug --target build_ros

if [ -n "${ZSH_VERSION:-}" ]; then
  _AH_ROS_DIR="$(cd "$(dirname "${(%):-%x}")" && pwd)"
elif [ -n "${BASH_VERSION:-}" ]; then
  _AH_ROS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
else
  _AH_ROS_DIR="$(cd "$(dirname "$0")" && pwd)"
fi

# Overlay: zsh must use setup.zsh (setup.bash breaks COLCON_CURRENT_PREFIX under zsh).
if [ -n "${ZSH_VERSION:-}" ]; then
  # shellcheck disable=SC1091
  . "${_AH_ROS_DIR}/install/setup.zsh"
else
  # shellcheck disable=SC1091
  . "${_AH_ROS_DIR}/install/setup.bash"
fi

# Runtime env from aerohub_settings.ini (AhCommon) — domain, RMW, models path.
_AH_EXPORTS=""
for _AH_HELPER in \
  "${_AH_ROS_DIR}/install/ah_common/lib/ah_common/ah_settings_shell_exports" \
  "${_AH_ROS_DIR}/install/ah_common/bin/ah_settings_shell_exports" \
  "${_AH_ROS_DIR}/../build/Debug/AhCommon/ah_settings_shell_exports" \
  "${_AH_ROS_DIR}/../build/Debug/AhCommon/ah_settings_shell_exports.app/Contents/MacOS/ah_settings_shell_exports"
do
  if [ -x "${_AH_HELPER}" ]; then
    _AH_EXPORTS="$("${_AH_HELPER}" 2>/dev/null)" || true
    break
  fi
done

if [ -n "${_AH_EXPORTS}" ]; then
  # shellcheck disable=SC2086
  eval ${_AH_EXPORTS}
else
  echo "[ah_ros] warning: ah_settings_shell_exports not found — env not loaded from settings." >&2
  echo "[ah_ros]   Build the ROS stack (cmake --build … --target build_ros) then re-source this script." >&2
  echo "[ah_ros]   CWD must contain aerohub_settings.ini (typically aero-hub/)." >&2
fi

echo "[ah_ros] overlay=${_AH_ROS_DIR}/install  ROS_DOMAIN_ID=${ROS_DOMAIN_ID:-unset}  RMW=${RMW_IMPLEMENTATION:-unset}  YOLO_MODELS=${AERO_HUB_YOLO_MODELS:-unset}"
unset _AH_ROS_DIR _AH_HELPER _AH_EXPORTS
