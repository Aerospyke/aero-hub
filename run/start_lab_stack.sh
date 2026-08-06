#!/usr/bin/env bash
# Lab stack in one tmux window (panes for ROS nodes + optional dashboard).
#
#   cd …/aero-hub/run
#   ./start_lab_stack.sh              # core + yolo + dashboard (default)
#   ./start_lab_stack.sh --no-yolo    # core + dashboard only
#   ./start_lab_stack.sh --ros-only   # core + yolo only (no dashboard)
#   ./start_lab_stack.sh --ros-only --no-yolo   # core only
#
# Requires: tmux (e.g. brew install tmux). Works inside Ghostty / Terminal / iTerm —
# tmux is the multiplexer; the outer app is just the host terminal.
#
set -eo pipefail

AH_LAB_RUN_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "${AH_LAB_RUN_DIR}"

AH_SESSION="${AH_TMUX_SESSION:-aerohub}"
AH_DASH_BIN="${AH_LAB_RUN_DIR}/AeroHub.app/Contents/MacOS/AeroHub"

_WITH_YOLO=1
_WITH_DASH=1
for _arg in "$@"; do
  case "${_arg}" in
    --no-yolo) _WITH_YOLO=0 ;;
    --ros-only|ros-only) _WITH_DASH=0 ;;
    -h|--help)
      echo "Usage: $0 [--no-yolo] [--ros-only]"
      echo "  Starts a tmux session '${AH_SESSION}' with panes for ah_core,"
      echo "  ah_yolo (default), and the dashboard (source init in each pane)."
      echo "  --no-yolo   omit the ah_yolo pane"
      echo "  --ros-only  omit the dashboard (ROS nodes only)"
      exit 0
      ;;
  esac
done

if ! command -v tmux >/dev/null 2>&1; then
  echo "error: tmux not found. Install with:  brew install tmux" >&2
  exit 127
fi

if [[ "${_WITH_DASH}" -eq 1 && ! -x "${AH_DASH_BIN}" ]]; then
  echo "error: dashboard not found at ${AH_DASH_BIN}" >&2
  echo "  Build and install:  cmake --build build/Debug --target AeroHub && cmake --install build/Debug" >&2
  echo "  Or use:  $0 --ros-only" >&2
  exit 1
fi

# Already running: attach (or switch if nested).
if tmux has-session -t "=${AH_SESSION}" 2>/dev/null; then
  echo "[start_lab_stack] session '${AH_SESSION}' already exists — attaching"
  if [[ -n "${TMUX:-}" ]]; then
    exec tmux switch-client -t "=${AH_SESSION}"
  else
    exec tmux attach-session -t "=${AH_SESSION}"
  fi
fi

# Each pane: stay in run/, source init, run the process. On exit, keep the pane open.
_AH_INIT="source ./init_ah_ros_in_terminal.sh"
_AH_CORE="${_AH_INIT} && ros2 run ah_core ah_core_node; echo '[ah_core exited]'; exec bash"
_AH_YOLO="${_AH_INIT} && ros2 run ah_yolo ah_yolo_node; echo '[ah_yolo exited]'; exec bash"
_AH_DASH="${_AH_INIT} && exec ./AeroHub.app/Contents/MacOS/AeroHub"

# Stack panes top → bottom: core, [yolo], [dashboard]
tmux new-session -d -s "${AH_SESSION}" -n lab -c "${AH_LAB_RUN_DIR}" "${_AH_CORE}"

if [[ "${_WITH_YOLO}" -eq 1 ]]; then
  tmux split-window -v -t "=${AH_SESSION}:lab" -c "${AH_LAB_RUN_DIR}" "${_AH_YOLO}"
fi
if [[ "${_WITH_DASH}" -eq 1 ]]; then
  tmux split-window -v -t "=${AH_SESSION}:lab" -c "${AH_LAB_RUN_DIR}" "${_AH_DASH}"
fi
tmux select-layout -t "=${AH_SESSION}:lab" even-vertical
tmux select-pane -t "=${AH_SESSION}:lab.0"

_AH_PANES="core"
[[ "${_WITH_YOLO}" -eq 1 ]] && _AH_PANES="${_AH_PANES}, yolo"
[[ "${_WITH_DASH}" -eq 1 ]] && _AH_PANES="${_AH_PANES}, dashboard"
echo "[start_lab_stack] tmux session '${AH_SESSION}' — panes: ${_AH_PANES}"
echo "  detach: Ctrl-b d    reattach: tmux attach -t ${AH_SESSION}"

if [[ -n "${TMUX:-}" ]]; then
  exec tmux switch-client -t "=${AH_SESSION}"
else
  exec tmux attach-session -t "=${AH_SESSION}"
fi
