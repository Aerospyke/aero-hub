# Source this in every terminal that talks to the AeroHub ROS graph (bash or zsh).
#
#   cd …/aero-hub/run
#   source ./init_ah_ros_in_terminal.sh
#
# Then, in that same shell (do not use "open AeroHub.app" — it drops ROS env):
#   ros2 run ah_core ah_core_node
#   ros2 run ah_yolo ah_yolo_node
#   ./AeroHub.app/Contents/MacOS/AeroHub
#
# This directory is the operational CWD:
#   - aerohub_settings.ini (AhCommon, CWD only)
#   - yolo_models_dir in INI (default ../yolo-models → real aero-hub/yolo-models/)
#   - bin/ah_settings_shell_exports (installed by build_ros)
#
# This only:
#   1) cds to this run/ directory
#   2) loads the colcon overlay (../ros/install)
#   3) exports ROS_DOMAIN_ID, RMW_IMPLEMENTATION, AERO_HUB_YOLO_MODELS from the INI
#
# Namespace is NOT exported: each node applies it from settings.
#
# If bin/ah_settings_shell_exports is missing:
#   ./ros/scripts/build_ros.sh
#   # or:  cmake --build …/build/Debug --target build_ros

if [ -n "${ZSH_VERSION:-}" ]; then
  _AH_RUN_DIR="$(cd "$(dirname "${(%):-%x}")" && pwd)"
elif [ -n "${BASH_VERSION:-}" ]; then
  _AH_RUN_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
else
  _AH_RUN_DIR="$(cd "$(dirname "$0")" && pwd)"
fi

# Operational root: settings and relative model paths resolve here.
cd "${_AH_RUN_DIR}" || return 1 2>/dev/null || exit 1

_AH_ROS_INSTALL="${_AH_RUN_DIR}/../ros/install"
if [ ! -f "${_AH_ROS_INSTALL}/setup.zsh" ] && [ ! -f "${_AH_ROS_INSTALL}/setup.bash" ]; then
  echo "[ah_ros] error: colcon overlay not found at ${_AH_ROS_INSTALL}" >&2
  echo "[ah_ros]   Build first:  cmake --build …/build/Debug --target build_ros" >&2
  unset _AH_RUN_DIR _AH_ROS_INSTALL
  return 1 2>/dev/null || exit 1
fi

# Overlay: zsh must use setup.zsh (setup.bash breaks COLCON_CURRENT_PREFIX under zsh).
if [ -n "${ZSH_VERSION:-}" ]; then
  # shellcheck disable=SC1091
  . "${_AH_ROS_INSTALL}/setup.zsh"
else
  # shellcheck disable=SC1091
  . "${_AH_ROS_INSTALL}/setup.bash"
fi

_AH_HELPER="${_AH_RUN_DIR}/bin/ah_settings_shell_exports"
if [ ! -x "${_AH_HELPER}" ]; then
  echo "[ah_ros] error: ${_AH_HELPER} not found or not executable." >&2
  echo "[ah_ros]   Populate run/:  ./ros/scripts/build_ros.sh   (or cmake --build … --target build_ros)" >&2
  unset _AH_RUN_DIR _AH_ROS_INSTALL _AH_HELPER
  return 1 2>/dev/null || exit 1
fi

_AH_EXPORTS="$("${_AH_HELPER}" 2>/dev/null)" || true
if [ -n "${_AH_EXPORTS}" ]; then
  # shellcheck disable=SC2086
  eval ${_AH_EXPORTS}
else
  echo "[ah_ros] warning: ah_settings_shell_exports produced no exports." >&2
  echo "[ah_ros]   AhCommon will create aerohub_settings.ini with defaults on first load if missing." >&2
fi

echo "[ah_ros] cwd=${_AH_RUN_DIR}  overlay=${_AH_ROS_INSTALL}  ROS_DOMAIN_ID=${ROS_DOMAIN_ID:-unset}  RMW=${RMW_IMPLEMENTATION:-unset}  YOLO_MODELS=${AERO_HUB_YOLO_MODELS:-unset}"
unset _AH_RUN_DIR _AH_ROS_INSTALL _AH_HELPER _AH_EXPORTS
