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

## Target environment: lab ROS install (macOS + Linux)

AeroHub is a **lab stack**, not a self-contained consumer app.

| Expectation | Meaning |
|-------------|---------|
| **ROS is already installed** | RoboStack (`ros_env`) and/or system ROS 2 Jazzy (`/opt/ros/jazzy`, etc.) |
| **Workspace overlay** | This repo’s `ros/install` (colcon) sits on top of that distro |
| **Runtime uses the lab install** | RMW plugins, typesupport, OpenCV, etc. come from the ROS prefix — we do **not** ship a full private ROS tree inside the dashboard package |
| **Operational root** | Always [`run/`](run/) as CWD (settings, init script, optional installed UI) |

**macOS:** RoboStack + host UI/core (same machine) is the usual demo. Docker Desktop on Mac still does not bridge DDS to the host for Mac UI ↔ container core (vault Task_26).

**Linux:** Same lab model — native Jazzy or RoboStack; colcon overlay; run from `run/`. Dashboard is an ELF (or whatever CMake produces), not `AeroHub.app`. Prefer a sourced ROS env (or the small ament/rpath lab hooks) rather than AppImage-style ROS bundling.

### Lab baseline

| Setting | Value |
|---------|--------|
| Distro | ROS 2 **Jazzy** (RoboStack and/or system install) |
| RMW | `rmw_fastrtps_cpp` (from lab install; also in `run/aerohub_settings.ini`) |
| Domain | **42** (INI / AhCommon; init script exports for CLI) |
| Namespace | empty = root graph `/ah/...` (optional multi-drone: `namespace=uav1` → `/uav1/ah/...`) |
| Platforms | **macOS** and **Linux** lab machines |

### Prerequisites

1. **macOS or Linux** lab host with a working ROS 2 Jazzy install (RoboStack `ros_env` is the tested Mac path).
2. **Qt 6** for the dashboard (edit `CMakeLists.txt` `QT_INSTALL_LOCATION` / `QT_VERSION_TO_USE` on Mac as needed).
3. **CMake** 3.16+.
4. **OpenCV** available to the same env that builds `ah_core`.
5. **Optional:** Conan (Path B / CLion profiles only). `conanfile.py` has **no package requirements** today.

**Operational CWD is [`run/`](run/).** Settings: `run/aerohub_settings.ini` only (AhCommon writes defaults if missing). Always **`source`** `run/init_ah_ros_in_terminal.sh` (do not execute it).

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

After either path, `ros/install` exists and the dashboard is installed under `run/` (`AeroHub.app` on macOS). The UI expects a **lab ROS install** (RoboStack / system Jazzy) for RMW and plugins — not a self-contained ROS tree inside the app.

Iterative rebuild:

```bash
cmake --build build/Debug --target AeroHub
cmake --install build/Debug
# ROS only:  ./ros/scripts/build_ros.sh
# or after Dashboard is configured:  cmake --build build/Debug --target build_ros
```

---

### Run the stack

**Rules**

1. **CWD = `run/`** for every process (settings file is CWD-only).
2. **Every terminal:** `source ./init_ah_ros_in_terminal.sh` (do **not** execute it with `./`).
3. **Dashboard:** run the binary **in that same shell**. Do **not** use `open ./AeroHub.app` — macOS `open` does not inherit the shell’s ROS env and will crash or mis-configure.

**Terminal setup (each role):**

```bash
conda activate ros_env          # or: source /opt/ros/jazzy/setup.bash on Linux
cd /path/to/aero-hub/run
source ./init_ah_ros_in_terminal.sh
```

| Terminal | Command |
|----------|---------|
| A — core | `ros2 run ah_core ah_core_node` |
| B — YOLO (optional) | `ros2 run ah_yolo ah_yolo_node` |
| C — dashboard | `./start_dashboard.sh` (macOS; sources init then runs the app binary) |

**One-shot (tmux):** from `run/`, `./start_lab_stack.sh` starts panes for `ah_core`, `ah_yolo`, and the dashboard (`--no-yolo` / `--ros-only` to drop YOLO or the dashboard). Works inside Ghostty/Terminal/iTerm; needs `tmux`.
**CLion:** env `ros_env` active; Run configuration Working directory = **`aero-hub/run`**; target **AeroHub**. Do not use bare `open ./AeroHub.app`.

Settings: **`run/aerohub_settings.ini`** only (AhCommon creates defaults if missing: domain **42**, `yolo_models_dir=../yolo-models`, …). See [`run/README.md`](run/README.md).

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
