# Deprecated location. Prefer:
#   cd …/aero-hub/run && source ./init_ah_ros_in_terminal.sh
#
# This wrapper sources the run/ script (which cds into run/ for settings).

if [ -n "${ZSH_VERSION:-}" ]; then
  _AH_SCRIPTS_DIR="$(cd "$(dirname "${(%):-%x}")" && pwd)"
elif [ -n "${BASH_VERSION:-}" ]; then
  _AH_SCRIPTS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
else
  _AH_SCRIPTS_DIR="$(cd "$(dirname "$0")" && pwd)"
fi

_AH_RUN_INIT="${_AH_SCRIPTS_DIR}/../../run/init_ah_ros_in_terminal.sh"
if [ ! -f "${_AH_RUN_INIT}" ]; then
  echo "[ah_ros] error: ${_AH_RUN_INIT} missing." >&2
  unset _AH_SCRIPTS_DIR _AH_RUN_INIT
  return 1 2>/dev/null || exit 1
fi

echo "[ah_ros] note: ros/scripts/init_ah_ros_in_terminal.sh is deprecated; use run/init_ah_ros_in_terminal.sh" >&2
# shellcheck disable=SC1090
. "${_AH_RUN_INIT}"
unset _AH_SCRIPTS_DIR _AH_RUN_INIT
