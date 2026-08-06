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
| `init_ah_ros_in_terminal.sh` | Overlay + env from INI (**source** every terminal; do not `./`) |
| `AeroHub.app` | Dashboard (macOS), from `cmake --install` |

Build trees (`build/`, `ros/build/`, `ros/install/`) stay outside; the init script sources **`../ros/install`** with a single fixed path.

`CMAKE_INSTALL_PREFIX` is **`aero-hub/run`** so a normal install deploys the dashboard here.

**Order:** build ROS (`./ros/scripts/build_ros.sh`) **before** configuring the Dashboard (needs `ah_msgs` in `ros/install`).

---

## From-scratch: Path A — plain CMake (no Conan)

```bash
cd /path/to/aero-hub
# Optional clean:
# rm -rf ros/build ros/install ros/log build/Debug run/bin run/AeroHub.app

conda activate ros_env
./ros/scripts/build_ros.sh

cmake -S . -B build/Debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build/Debug --target AeroHub
cmake --install build/Debug   # → run/AeroHub.app
```

---

## From-scratch: Path B — Conan toolchain (optional)

`conanfile.py` has no package deps today; it only generates toolchain/presets.

```bash
cd /path/to/aero-hub
# Optional clean (same as Path A)

conan install -pr=clang-debug
conda activate ros_env
./ros/scripts/build_ros.sh

cmake --preset conan-debug
cmake --build build/Debug --target AeroHub
cmake --install build/Debug   # → run/AeroHub.app
```

---

## Run (after either path)

**Every terminal:**

```bash
conda activate ros_env
cd /path/to/aero-hub/run
source ./init_ah_ros_in_terminal.sh

ros2 run ah_core ah_core_node
ros2 run ah_yolo ah_yolo_node
open ./AeroHub.app
```

**From IDE:** CLion **Working directory** = `…/aero-hub/run`
