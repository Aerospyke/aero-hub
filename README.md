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
3. **Qt 6.11** (or edit `CMakeLists.txt` `QT_INSTALL_LOCATION` / `QT_VERSION_TO_USE` to match your install).
4. **CMake** 3.16+, **Conan** if your CLion profile uses the existing Conan toolchain (optional for a plain CMake configure).
5. **OpenCV** available to the RoboStack / system environment used to build `ah_core` (RoboStack/desktop images usually provide it).

### 1. Build ROS packages (`ah_msgs` + `ah_core`)

Dashboard links against **`ros/install`** for `ah_msgs`. Build this first.

```bash
conda activate ros_env
cd /path/to/aero-hub/ros

# Command line: no extra compiler flags needed.
# Do NOT use --symlink-install for this lab (breaks remote IDE indexing).
colcon build --packages-select ah_msgs ah_core
source install/setup.bash
```

Optional (CLion only): if the Compilation Database fails on conda’s triple-prefixed clang, regenerate with system compilers:

```bash
colcon build --packages-select ah_msgs ah_core --cmake-args \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
  -DCMAKE_C_COMPILER=/usr/bin/clang \
  -DCMAKE_CXX_COMPILER=/usr/bin/clang++
```

Smoke-test core alone:

```bash
export ROS_DOMAIN_ID=42
ros2 run ah_core ah_core_node
# other terminal (same env + source install/setup.bash + domain 42):
ros2 topic echo /ah/system/status --once
ros2 node list   # expect /ah_core
```

More detail (Docker build of the same packages): [`ros/README.md`](ros/README.md).

### 2. Settings file (domain / namespace)

Working directory for the app should be able to see **`aerohub_settings.ini`** (often the `aero-hub/` project root as CLion CWD).

- Template: [`aerohub_settings_template.ini`](aerohub_settings_template.ini)
- If missing, the app writes defaults (including `domain_id=42`, empty `namespace`)

```ini
[ROS]
domain_id=42
namespace=
; namespace=uav1   # multi-vehicle: dashboard + core must match
```

**CLI tools** (`ros2 topic …`) do **not** read that INI; for them:

```bash
export ROS_DOMAIN_ID=42
# if using a namespace, topic names are /uav1/ah/... not /ah/...
```

### 3. Build the dashboard (plain CMake + Qt)

**Not colcon.** The **verified lab path** is **CLion** with RoboStack + the existing Conan/CMake profile (this repo’s `build/Debug` uses Conan’s toolchain, not a bare `CMAKE_PREFIX_PATH`-only configure).

**CLion (primary):**

```bash
conda activate ros_env
open -na "CLion.app"
```

- Open the `aero-hub` project; use your Debug (Conan) CMake profile.  
- Build target **AeroHub**.  
- Cold-start from Dock: root `CMakeLists.txt` falls back to `~/miniconda3/envs/ros_env` if `CONDA_PREFIX` is unset (still prefer launching from `ros_env`).  
- Working directory for Run: `aero-hub/` (so `aerohub_settings.ini` is found).

**Command-line rebuild** of an **already configured** tree (what iterative CLI builds use after CLion has configured once):

```bash
conda activate ros_env
cd /path/to/aero-hub
cmake --build build/Debug --target AeroHub -j"$(sysctl -n hw.ncpu)"
```

A **fresh** CLI configure from scratch is not the documented lab path: this project’s Debug/Release dirs are set up with **Conan** (`CMakeUserPresets.json` → `build/*/generators/conan_toolchain.cmake`). Re-create that via CLion or your usual Conan + CMake flow; do not assume a one-liner `-DCMAKE_PREFIX_PATH="$CONDA_PREFIX;…/Qt"` is equivalent unless you have verified it on your machine.

**Requires:** successful `ros/install` (for `ah_msgs`) so track services link. Qt path is set in root `CMakeLists.txt` (`QT_INSTALL_LOCATION`).

Binary (Debug example):

```text
build/Debug/DashboardApp/AeroHub.app/Contents/MacOS/AeroHub
```

### 4. Run the Milestone_1 demo (two processes, same host)

**Terminal A — core**

```bash
conda activate ros_env
cd /path/to/aero-hub/ros
source install/setup.bash
export ROS_DOMAIN_ID=42
ros2 run ah_core ah_core_node
```

**Terminal B — dashboard**

```bash
conda activate ros_env
export ROS_DOMAIN_ID=42   # optional if settings file has domain_id=42
# CWD = aero-hub/ so aerohub_settings.ini is found (or set CLion working directory)
cd /path/to/aero-hub
./build/Debug/DashboardApp/AeroHub.app/Contents/MacOS/AeroHub
```

Or run **AeroHub** from CLion with the same env / working directory.

### 5. Manual test script (acceptance)

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
  AhCommon/                   # C++17 std-only: settings, Trim, SanitizeNamespace
  DashboardApp/               # UI + rclcpp bridge (C++23)
  FlightInstruments/          # QML instruments module
  aerohub_settings_template.ini
  ros/                        # colcon: ah_msgs, ah_core, ah_yolo
    README.md
```

**Settings:** `ah::Settings` in AhCommon — e.g. `settings.ros().domainId()`, `settings.camera().selection()`,
`settings.jsbSim().get("ports/input")`. Dashboard and `ah_core` share this (no Qt in common).

**Optional ROS rebuild from CMake** (not required for every app build):

```bash
cmake --build build/Debug --target build_ros
```

---

## Troubleshooting

| Symptom | Likely cause |
|---------|----------------|
| `No 'rosidl_typesupport_c' found` | CMake without RoboStack env — activate `ros_env` or use cold-start path in root `CMakeLists.txt` |
| `find_package(ah_msgs)` fails | Build `ros/` first (`colcon` → `install/`) |
| `ros2` does not see app / core | Different `ROS_DOMAIN_ID` or **namespace** between processes |
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
weights under `models/` keep their own terms — see `NOTICE`.

---

## Documentation map

| Topic | Where |
|-------|--------|
| Lab ROS notes | Vault `reference/ros-lab-notes.md` |
| Topics / services contract | Vault `reference/ros2-interface-map.md` |
| Milestone_1 acceptance | Vault `milestones/milestone_1/acceptance.md` |
| Task board | Vault `milestones/milestone_1/kanban.md` |
| Colcon / Docker details | [`ros/README.md`](ros/README.md) |
