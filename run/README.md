# AeroHub `run/` — operational directory

Lab root for day-to-day work: ROS nodes, CLI, and dashboard. **CWD should be this directory.**

## Lab model (macOS + Linux)

This tree is **not** a self-contained product image. It assumes:

1. A **ROS 2 Jazzy lab install** (RoboStack `ros_env` and/or `/opt/ros/jazzy`, etc.)
2. This repo’s **colcon overlay** at `../ros/install`
3. You **`source ./init_ah_ros_in_terminal.sh`** in each terminal (do not `./` it)

AhCommon loads **`./aerohub_settings.ini` from CWD only**. Missing file → built-in defaults written here.  
YOLO weights: `[ROS] yolo_models_dir` (default `../yolo-models`).

Dashboard install may vendor **Qt** into the app bundle (macOS) and add **rpath / ament hooks** so the process can see the lab ROS install. It does **not** replace installing ROS on the machine.

## Layout

| Path | Role |
|------|------|
| `aerohub_settings.ini` | Runtime policy (created by AhCommon if missing) |
| `bin/ah_settings_shell_exports` | From `build_ros` (env exports from INI) |
| `init_ah_ros_in_terminal.sh` | Overlay + env from INI (**source** every terminal) |
| `AeroHub.app` / binary | Dashboard after `cmake --install` (platform-specific) |

**Order:** `./ros/scripts/build_ros.sh` **before** Dashboard CMake configure (`ah_msgs`).

---

## From-scratch: Path A — plain CMake (no Conan)

```bash
cd /path/to/aero-hub
# Optional clean:
# rm -rf ros/build ros/install ros/log build/Debug run/bin run/AeroHub.app

conda activate ros_env   # or: source /opt/ros/jazzy/setup.bash on Linux
./ros/scripts/build_ros.sh

cmake -S . -B build/Debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build/Debug --target AeroHub
cmake --install build/Debug
```

---

## From-scratch: Path B — Conan toolchain (optional)

`conanfile.py` has no package deps today; it only generates toolchain/presets.

```bash
cd /path/to/aero-hub
conan install -pr=clang-debug
conda activate ros_env
./ros/scripts/build_ros.sh

cmake --preset conan-debug
cmake --build build/Debug --target AeroHub
cmake --install build/Debug
```

---

## Run (macOS or Linux lab)

**Every terminal:**

```bash
conda activate ros_env   # or source system ROS setup on Linux
cd /path/to/aero-hub/run
source ./init_ah_ros_in_terminal.sh

ros2 run ah_core ah_core_node
ros2 run ah_yolo ah_yolo_node
```

**Dashboard**

```bash
# macOS (after install):
open ./AeroHub.app
# or: ./AeroHub.app/Contents/MacOS/AeroHub

# Linux (after install; path may be run/bin/AeroHub or similar):
# ./AeroHub
```

**IDE:** Working directory = `…/aero-hub/run`, with the same ROS env active.
