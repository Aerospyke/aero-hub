#!/usr/bin/env bash
# Launch the AeroHub dashboard in this shell (lab: needs ROS env + run/ CWD).
#
#   cd …/aero-hub/run
#   ./start_dashboard.sh
#
set -eo pipefail
# Note: do not use `set -u` — colcon setup.bash references optional unbound vars.

# Capture before sourcing init (init unsets _AH_RUN_DIR).
AH_LAB_RUN_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "${AH_LAB_RUN_DIR}"

# shellcheck disable=SC1091
source "${AH_LAB_RUN_DIR}/init_ah_ros_in_terminal.sh"

AH_DASH_BIN="${AH_LAB_RUN_DIR}/AeroHub.app/Contents/MacOS/AeroHub"
if [[ ! -x "${AH_DASH_BIN}" ]]; then
  echo "error: dashboard not found at ${AH_DASH_BIN}" >&2
  echo "  Build and install:  cmake --build build/Debug --target AeroHub && cmake --install build/Debug" >&2
  exit 1
fi

exec "${AH_DASH_BIN}" "$@"
