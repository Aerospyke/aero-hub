# AeroHub `run/` — operational directory

Lab root for day-to-day work: ROS nodes, CLI, and dashboard. **Always use this directory as CWD.**

## Lab model (macOS + Linux)

Not a self-contained product image. Assumes:

1. A **ROS 2 Jazzy lab install** (RoboStack `ros_env` and/or `/opt/ros/jazzy`, etc.)
2. This repo’s **colcon overlay** at `../ros/install`
3. **`source ./init_ah_ros_in_terminal.sh`** in every terminal (never `./init_…`)

| File | Role |
|------|------|
| `aerohub_settings.ini` | Runtime policy (AhCommon creates defaults if missing) |
| `bin/ah_settings_shell_exports` | From `build_ros` (env from INI) |
| `init_ah_ros_in_terminal.sh` | Overlay + domain / RMW / YOLO path from INI |
| `AeroHub.app` | Dashboard after `cmake --install` (macOS) |

Settings are **CWD-only**. YOLO weights: `[ROS] yolo_models_dir` (default `../yolo-models`).

**Build order:** `./ros/scripts/build_ros.sh` before Dashboard CMake configure (`ah_msgs`).

---

## Build (summary)

```bash
cd /path/to/aero-hub
conda activate ros_env    # or source system ROS on Linux

./ros/scripts/build_ros.sh
cmake -S . -B build/Debug -DCMAKE_BUILD_TYPE=Debug   # Path A: no Conan
cmake --build build/Debug --target AeroHub
cmake --install build/Debug
```

Optional Conan: `conan install -pr=clang-debug` then `cmake --preset conan-debug` instead of bare `-S/-B`. Full detail: [`../README.md`](../README.md).

---

## Run the stack

### Every terminal (required)

```bash
conda activate ros_env    # or: source /opt/ros/jazzy/setup.bash
cd /path/to/aero-hub/run
source ./init_ah_ros_in_terminal.sh
```

Confirm the echo line: `ROS_DOMAIN_ID`, `RMW`, `YOLO_MODELS` set.

### Processes (separate terminals, each after the setup above)

| Role | Command |
|------|---------|
| **Core** | `ros2 run ah_core ah_core_node` |
| **YOLO** (optional) | `ros2 run ah_yolo ah_yolo_node` |
| **Dashboard (macOS)** | `./start_dashboard.sh`  (or `./AeroHub.app/Contents/MacOS/AeroHub` after `source`) |
| **Dashboard (Linux)** | installed binary under `run/` (e.g. `./AeroHub` or `./bin/AeroHub`) |

### Helpers

```bash
./start_dashboard.sh              # source init + dashboard in this shell
./start_lab_stack.sh              # tmux: core | yolo | dashboard (default)
./start_lab_stack.sh --no-yolo    # tmux: core | dashboard only
./start_lab_stack.sh --ros-only   # tmux: core | yolo only (no dashboard)
```

`start_lab_stack.sh` uses **tmux** (one window, multiple panes) so it works in **Ghostty**, Terminal, iTerm, etc. Requires `tmux` on PATH (`brew install tmux`).

```text
reattach later:  tmux attach -t aerohub
detach:          Ctrl-b d
```
### Do **not**

```bash
./init_ah_ros_in_terminal.sh     # wrong — must source (start_dashboard.sh does this for you)
open ./AeroHub.app               # wrong — no shell ROS env
```

### CLion

- Activate **`ros_env`** before opening the IDE (or ensure the Run env has it).
- Target: **AeroHub**
- **Working directory:** `…/aero-hub/run`
- You can run the build-tree binary from CLion; no need to `open` the installed `.app`.

---

## Quick smoke check

With core (and optionally dashboard) up:

```bash
# same shell setup as above
echo $ROS_DOMAIN_ID
ros2 node list
ros2 topic echo /ah/system/status --once
```
