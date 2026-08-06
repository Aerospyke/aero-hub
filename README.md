# AeroHub (code)

Modular companion stack:

| Piece | Role |
|-------|------|
| **DashboardApp** | Qt 6 / QML UI + in-process `rclcpp` (`ah_dashboard`) |
| **FlightInstruments** | QML flight instruments module |
| **ros/** | Colcon workspace: `ah_msgs`, `ah_core` stub |

Primary project docs (decisions, tasks, interface map) live in the Obsidian vault:

```text
~/Documents/obsidian-vault/projects/aerohub/
```

---

## Milestone_1 — build & run (demo path)

This is the **supported end-to-end path on macOS**: dashboard and `ah_core` both on the **host** under **RoboStack**, same ROS domain (default **42**).

**Why not Mac UI ↔ Docker `ah_core` for the demo?** Docker Desktop on macOS puts containers on a bridge network; default DDS discovery does not reach the host. That is tracked as vault Task_26. Use Docker for **Linux builds** of `ah_core` if you want; for the **live UI demo**, run core on the Mac.

### Lab baseline

| Setting | Value |
|---------|--------|
| Distro | ROS 2 **Jazzy** (RoboStack `ros_env` on Mac) |
| RMW | `rmw_fastrtps_cpp` (usually the default; export only if needed) |
| Domain | **42** (settings file and/or `export ROS_DOMAIN_ID=42` for CLI) |
| Namespace | empty = root graph `/ah/...` (optional multi-drone: `namespace=uav1` → `/uav1/ah/...`) |

### Prerequisites

1. **macOS** lab machine (arm64 tested).
2. **RoboStack Jazzy** conda env (example name: `ros_env`).  
   See [RoboStack Getting Started](https://robostack.github.io/GettingStarted.html).
3. **Qt 6.11** (or edit `CMakeLists.txt` `QT_INSTALL_LOCATION` / `QT_VERSION_TO_USE`).
4. **CMake** 3.16+.
5. **OpenCV** available in the RoboStack env used to build `ah_core`.
6. **Optional:** Conan (only if you use Path B / a Conan CLion profile). `conanfile.py` currently has **no package requirements** — it only generates a toolchain/layout.

**Operational CWD is [`run/`](run/).** Settings: `run/aerohub_settings.ini` only (created by AhCommon with defaults if missing). Always **`source`** `run/init_ah_ros_in_terminal.sh` (do not execute it).

**Order matters:** build ROS (`ros/install` + `ah_msgs`) **before** configuring the Dashboard.

Optional clean:

```bash
cd /path/to/aero-hub
rm -rf ros/build ros/install ros/log build/Debug run/bin run/AeroHub.app
```

---

### Path A — plain CMake (no Conan)

Recommended for terminal builds. Qt and install prefix come from root `CMakeLists.txt`; activate `ros_env` so ROS is found.

```bash
cd /path/to/aero-hub
conda activate ros_env

# 1) ROS first (colcon → ros/install, run/bin/ah_settings_shell_exports)
./ros/scripts/build_ros.sh

# 2) Configure + build + install Dashboard (CMAKE_INSTALL_PREFIX = run/)
cmake -S . -B build/Debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build/Debug --target AeroHub
cmake --install build/Debug
```

Release: use `build/Release` and `-DCMAKE_BUILD_TYPE=Release`.

---

### Path B — Conan toolchain (optional)

Useful if you keep Conan profiles / CLion presets. Still **no Conan dependencies** today — only generates `build/*/generators` and CMake presets.

```bash
cd /path/to/aero-hub
conan install -pr=clang-debug
conda activate ros_env

# 1) ROS first (same as Path A)
./ros/scripts/build_ros.sh

# 2) Configure via Conan preset, then build + install
cmake --preset conan-debug
cmake --build build/Debug --target AeroHub
cmake --install build/Debug
```

Release: `conan install -pr=clang-release`, then `cmake --preset conan-release` and build/install under `build/Release`.

**CLion:** activate `ros_env`, open the project, use either a plain CMake profile (Path A) or a Conan Debug profile (Path B). Working directory for Run: **`aero-hub/run`**.

After either path, `ros/install` exists and `run/AeroHub.app` is installed. Iterative rebuild:

```bash
cmake --build build/Debug --target AeroHub
cmake --install build/Debug
# ROS only:  ./ros/scripts/build_ros.sh
# or after Dashboard is configured:  cmake --build build/Debug --target build_ros
```

---

### Run the stack

**Every terminal:**

```bash
conda activate ros_env
cd /path/to/aero-hub/run
source ./init_ah_ros_in_terminal.sh
```

| Role | Command |
|------|---------|
| Core | `ros2 run ah_core ah_core_node` |
| YOLO | `ros2 run ah_yolo ah_yolo_node` |
| Dashboard | `open ./AeroHub.app` |

Or run **AeroHub** from CLion with Working directory = `aero-hub/run`.

Settings live only under **`run/`** (`aerohub_settings.ini`; defaults include `domain_id=42`, `yolo_models_dir=../yolo-models`). More detail: [`run/README.md`](run/README.md), [`ros/README.md`](ros/README.md).

### Manual test script (acceptance)

With both processes running (domain **42**, matching **namespace**):

1. **Status** — Footer **ROS** green; Control page shows live fields (`video_status`, etc.). Banner hidden.  
2. **Video** — Ops → Video panel: synthetic frames move.  
3. **Start tracking** — Ops: drag a box on Video (or edit fields in Tracking panel under Video) → **Start**. Footer Tracking **ON**.  
4. **Stop** — Ops Tracking panel **Stop** → Tracking **OFF**.  
5. **Cancel** — **Cancel** hard-resets stub tracking/segmentation flags.  
6. **Disconnect** — Kill `ah_core`. Within ~2 s: amber/red banner, footer **ROS LOST**, video **stale**/dimmed; app does not crash.  
7. **Reconnect** — Restart `ah_core` → status/video return without restarting the dashboard.

CLI helpers (optional):

```bash
export ROS_DOMAIN_ID=42
ros2 node list                    # /ah_dashboard , /ah_core
ros2 topic list
ros2 service list | grep tracking
```

### Architecture sketch

```text
macOS host (RoboStack)
  ├── AeroHub (Qt)  node: ah_dashboard
  │     sub  ah/system/status , ah/video/compressed
  │     client ah/tracking/{start,stop,cancel}
  └── ah_core_node
        pub  ah/system/status , ah/video/compressed
        srv  ah/tracking/{start,stop,cancel}
  same ROS_DOMAIN_ID + same namespace
```

Absolute names with empty namespace: `/ah/system/status`, etc.  
With `namespace=uav1`: `/uav1/ah/system/status`, node `/uav1/ah_dashboard`.

**Important:** use **relative** graph names in code (`ah/...`). A leading `/` is absolute and **ignores** the node namespace.

---

## Docker (optional — build / Linux-only core)

Use when you want a Jazzy container for colcon, not for Mac↔container DDS demo:

See **[`ros/README.md`](ros/README.md)** (mount `aero-hub/ros` → `/aero-hub-ros`, build `ah_msgs` + `ah_core`, `ros2 run`).

Mac host **will not** normally see container topics (Docker Desktop bridge). Same-host path above is the Milestone_1 demo.

---

## Repo layout

```text
aero-hub/
  CMakeLists.txt              # Dashboard superbuild (Qt + AhCommon)
  AhCommon/                   # std-only: settings, Trim, SanitizeNamespace
  DashboardApp/               # UI + rclcpp bridge (C++23)
  FlightInstruments/          # QML instruments module
  run/                        # operational CWD (init, settings, bin/)
  ros/                        # colcon: ah_common, ah_msgs, ah_core, ah_yolo
    README.md
```

**Settings:** `ah::Settings` in AhCommon — e.g. `settings.Ros().DomainId()`, `settings.Camera().Selection()`,
`settings.JsbSim().Get("ports/input")`. Dashboard and `ah_core` share this (no Qt in common). Live file: **`run/aerohub_settings.ini`** (auto-created with defaults if missing).

**Optional ROS rebuild** (not required for every app build):

```bash
./ros/scripts/build_ros.sh
# or, after Dashboard CMake is configured:
cmake --build build/Debug --target build_ros
```

---

## Troubleshooting

| Symptom | Likely cause |
|---------|----------------|
| `No 'rosidl_typesupport_c' found` | CMake without RoboStack env — activate `ros_env` or use cold-start path in root `CMakeLists.txt` |
| `find_package(ah_msgs)` fails | Build `ros/` first (`colcon` → `install/`) |
| `ros2` does not see app / core | Different `ROS_DOMAIN_ID` or **namespace**; or ran `./init_…` instead of `source` |
| `Package 'ah_core' not found` | Overlay not in this shell — `source run/init_ah_ros_in_terminal.sh` |
| dyld `_PyExc_RuntimeError` | Dashboard must link `Python3::Python` (already in CMake) |
| SpinBox customize warnings on macOS | App sets `QQuickStyle` to **Basic** in `main.cpp` |
| Mac `ros2` cannot see Docker topics | Expected on Docker Desktop — use same-host demo or Task_26 later |
| CLion compiler type error | Generate compile commands with `/usr/bin/clang` / `clang++` |

---

## License

AeroHub source code in this directory is licensed under the
**Apache License, Version 2.0**. See [`LICENSE`](LICENSE) and
[`NOTICE`](NOTICE).

ROS packages declare the same SPDX id in `package.xml` (`Apache-2.0`).

Third-party runtimes (ROS 2, Qt, OpenCV, Ultralytics, etc.) and model
weights under `yolo-models/` keep their own terms — see `NOTICE`.

---

## Documentation map

| Topic | Where |
|-------|--------|
| Lab ROS notes | Vault `reference/ros-lab-notes.md` |
| Topics / services contract | Vault `reference/ros2-interface-map.md` |
| Milestone_1 acceptance | Vault `milestones/milestone_1/acceptance.md` |
| Task board | Vault `milestones/milestone_1/kanban.md` |
| Colcon / Docker details | [`ros/README.md`](ros/README.md) |
