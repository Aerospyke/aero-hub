#!/usr/bin/env bash
# Build helper: install runtime tools into aero-hub/run/.
# Not for interactive use — called by build_ros.sh (and CMake target populate_run).
#
# Settings: AhCommon creates run/aerohub_settings.ini with built-in defaults on first
# load if missing — this script does not copy or seed any INI.
#
set -euo pipefail

_AH_SCRIPTS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
_AH_ROS_DIR="$(cd "${_AH_SCRIPTS_DIR}/.." && pwd)"
_AH_ROOT="$(cd "${_AH_ROS_DIR}/.." && pwd)"
_AH_RUN_DIR="${_AH_ROOT}/run"
_AH_BIN_DIR="${_AH_RUN_DIR}/bin"
_AH_ROS_INSTALL="${_AH_ROS_DIR}/install"

mkdir -p "${_AH_BIN_DIR}"

# Single source after colcon: ros/install/ah_common/bin/ah_settings_shell_exports
_AH_SRC="${_AH_ROS_INSTALL}/ah_common/bin/ah_settings_shell_exports"
if [[ ! -x "${_AH_SRC}" ]]; then
  echo "[populate_run] error: ${_AH_SRC} not found or not executable." >&2
  echo "[populate_run]   Build ROS first:  cmake --build build/Debug --target build_ros" >&2
  exit 1
fi

cp -f "${_AH_SRC}" "${_AH_BIN_DIR}/ah_settings_shell_exports"
chmod +x "${_AH_BIN_DIR}/ah_settings_shell_exports"
echo "[populate_run] installed run/bin/ah_settings_shell_exports  (from ${_AH_SRC})"

echo "[populate_run] ready: cd ${_AH_RUN_DIR} && source ./init_ah_ros_in_terminal.sh"
