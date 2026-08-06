# AeroHub `run/` — operational directory

All day-to-day terminal work (ROS nodes, CLI, dashboard) should use **this directory as CWD**.

AhCommon loads **`./aerohub_settings.ini` from the process CWD only** (no parent search).

YOLO weights path is **`[ROS] yolo_models_dir`** in that INI (default `../yolo-models` → real `aero-hub/yolo-models/`), not a directory under `run/`.

If `aerohub_settings.ini` is missing, **AhCommon** writes built-in defaults into the CWD on first load.

## Layout

| Path | Role |
|------|------|
| `aerohub_settings.ini` | Runtime policy (created by AhCommon if missing) |
| `bin/ah_settings_shell_exports` | Installed by `build_ros` |
| `init_ah_ros_in_terminal.sh` | Overlay + env from INI (source every terminal) |
| `AeroHub.app` | Dashboard (macOS), from `cmake --install` |

Build trees (`build/`, `ros/build/`, `ros/install/`) stay outside; the init script sources **`../ros/install`** with a single fixed path.

`CMAKE_INSTALL_PREFIX` is **`aero-hub/run`** so a normal install deploys the dashboard here.

## Setup after build

```bash
conda activate ros_env
# from aero-hub:
cmake --build build/Debug --target build_ros
cmake --build build/Debug --target AeroHub
cmake --install build/Debug
# or Release tree the same way with build/Release
```

## Every terminal

```bash
conda activate ros_env
cd /path/to/aero-hub/run
source ./init_ah_ros_in_terminal.sh

ros2 run ah_core ah_core_node
# other terminal (same cd + source):
ros2 run ah_yolo ah_yolo_node
```

## Dashboard

**From IDE:** CLion **Working directory** = `…/aero-hub/run`

**Deployed (after install):**

```bash
cd /path/to/aero-hub/run
source ./init_ah_ros_in_terminal.sh
open ./AeroHub.app
# or:  ./AeroHub.app/Contents/MacOS/AeroHub
```
