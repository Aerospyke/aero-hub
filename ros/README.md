# AeroHub ROS workspace

Colcon workspace for ROS 2 packages (`ah_common`, `ah_msgs`, `ah_core`, `ah_yolo`, …).

**Lab baseline:** Jazzy · `rmw_fastrtps_cpp` · domain **42** (from `run/aerohub_settings.ini` via AhCommon)  
See vault: `projects/aerohub/reference/ros-lab-notes.md`.  
**Day-to-day ops (host):** [`../run/README.md`](../run/README.md).

## Layout

```
ros/                    # host path: …/aero-hub/ros
  scripts/
    build_ros.sh        # colcon + populate run/bin
    populate_runtime.sh
  src/
    ah_common/          # AhCommon install for colcon (lib + ah_settings_shell_exports)
    ah_msgs/            # StartTracking, ListCameras, SelectCamera, SetYoloProfile, …
    ah_core/            # status + video + track + camera list/select
    ah_yolo/            # Ultralytics YOLO → ah/detections (JSON)
  build/                # gitignored (created by colcon)
  install/              # gitignored
  log/                  # gitignored
```

Operational CWD for nodes and CLI is **`../run/`** (settings INI, init script, dashboard install). Do not rely on a settings file under `ros/`.

---

## Host lab (macOS RoboStack / Linux) — preferred

### Build

```bash
conda activate ros_env          # or: source /opt/ros/jazzy/setup.bash on Linux
cd …/aero-hub
./ros/scripts/build_ros.sh      # ah_common ah_msgs ah_core ah_yolo + run/bin helper

# YOLO deps (once per env):
pip install "ultralytics>=8.3.0"
```

Or from a configured Dashboard tree: `cmake --build build/Debug --target build_ros`.

**Do not use `--symlink-install`** for this lab (breaks remote IDE indexing).

### Run (from `run/`)

Every terminal that talks to the graph:

```bash
conda activate ros_env
cd …/aero-hub/run
source ./init_ah_ros_in_terminal.sh
```

That loads `../ros/install` and exports **domain / RMW / YOLO models path** from `aerohub_settings.ini`.  
Bare `source ros/install/setup.zsh` alone is not enough (CLI often stays on domain 0 → service calls hang).

| Role | Command |
|------|---------|
| Core | `ros2 run ah_core ah_core_node` |
| YOLO | `ros2 run ah_yolo ah_yolo_node` |
| Dashboard | `./start_dashboard.sh` (sources init, then app binary) |

**One-shot (tmux — Ghostty / Terminal / iTerm):**

```bash
cd …/aero-hub/run
./start_lab_stack.sh              # core + yolo + dashboard (default)
./start_lab_stack.sh --ros-only   # core + yolo only
./start_lab_stack.sh --no-yolo    # core + dashboard
```

Do **not** use bare `open ./AeroHub.app` (macOS `open` does not inherit the shell ROS env).  
Do **not** `./init_ah_ros_in_terminal.sh` without `source`.

Full detail: [`../run/README.md`](../run/README.md).

### Domain / second-shell footgun

`ah_core` reads domain from settings **inside the node**. Each extra shell must also `source ./init_ah_ros_in_terminal.sh` from `run/` or `ros2` stays on the wrong domain.

```bash
cd …/aero-hub/run
source ./init_ah_ros_in_terminal.sh
echo $ROS_DOMAIN_ID   # must match [ROS] domain_id (default 42)
ros2 service list | grep camera
ros2 service call /ah/camera/list ah_msgs/srv/ListCameras "{refresh: true}"
```

### Topics / services (stub graph)

| Topic / service | Type | Notes |
|-----------------|------|--------|
| `ah/system/status` | `std_msgs/String` (JSON) | ~10 Hz |
| `ah/video/compressed` | `sensor_msgs/CompressedImage` | live or synthetic JPEG |
| `ah/detections` | `std_msgs/String` (JSON) | from `ah_yolo` |
| `ah/tracking/start` | `ah_msgs/srv/StartTracking` | bbox [0,1] |
| `ah/tracking/stop` / `cancel` | `std_srvs/Trigger` | |
| `ah/camera/list` / `select` | `ah_msgs/srv/…` | no hard-coded device IDs |
| `ah/yolo/set_profile` | `ah_msgs/srv/SetYoloProfile` | tank \| coco80 |
| `ah/yolo/reload` | `std_srvs/Trigger` | |

**Namespace:** `[ROS] namespace=` in `run/aerohub_settings.ini`. Empty = `/ah/...`. Example `uav1` → `/uav1/ah/...`. All nodes + dashboard must match.

**Camera:** owned by `ah_core` (`video.source`, `camera.device_path`, …). Prefer `index:N` / device path over bare indices. macOS: grant **Camera** to the host app that owns the process (Ghostty, Terminal, CLion, …).

**`install_name_tool` noise on macOS:** ignore if colcon reports `Finished <<< …`. Real failure = non-zero exit or missing binary.

---

## Docker lab (optional)

Mount **only** this `ros/` directory at `/aero-hub-ros` (UI/dashboard still runs on the host under RoboStack for Mac demos).

### 1. Start container

```bash
docker run -it --rm --name ros2_dev \
  --cap-add=sys_ptrace \
  --env="DISPLAY=host.docker.internal:0" \
  --env="RMW_IMPLEMENTATION=rmw_fastrtps_cpp" \
  --env="ROS_DOMAIN_ID=42" \
  --volume /tmp/.X11-unix:/tmp/.X11-unix:rw \
  --volume ~/Documents/projects/pix-eagle-stack/aero-hub/ros:/aero-hub-ros \
  -p 2222:22 \
  conorco/ros:jazzy-desktop-full-with-clion
```

### 2. Build inside container

```bash
cd /aero-hub-ros
source /opt/ros/jazzy/setup.bash
colcon build --packages-select ah_msgs ah_core ah_yolo
# ah_common may also be needed depending on layout; host build_ros.sh builds the full set
source install/setup.bash
```

### 3. Run / verify inside container

```bash
source /opt/ros/jazzy/setup.bash
source /aero-hub-ros/install/setup.bash
export ROS_DOMAIN_ID=42
export RMW_IMPLEMENTATION=rmw_fastrtps_cpp

ros2 run ah_core ah_core_node
# second shell:
ros2 topic echo /ah/system/status --once
ros2 service call /ah/tracking/start ah_msgs/srv/StartTracking \
  "{x: 0.1, y: 0.1, width: 0.2, height: 0.2}"
ros2 service call /ah/camera/list ah_msgs/srv/ListCameras "{refresh: true}"
```

**Mac host ↔ Docker DDS:** default Docker Desktop networking usually **does not** discover host RoboStack nodes. Prefer same-host stack for UI demos (vault Task_26).

### Optional: `rqt_image_view` (XQuartz on Mac)

Host: `open -a XQuartz` · `xhost +localhost`  
Container: `ros2 run rqt_image_view rqt_image_view` → select **`/ah/video/compressed`**.

---

## YOLO (`ah_yolo`)

```bash
cd …/aero-hub/run
source ./init_ah_ros_in_terminal.sh

# offline smoke (paths relative to aero-hub when invoked that way):
# python ../ros/src/ah_yolo/scripts/smoke_infer.py --profile coco80 --image /path/to.jpg

ros2 run ah_yolo ah_yolo_node
# optional: -p profile:=tank
ros2 topic echo /ah/detections --once
# ros2 service call /ah/yolo/reload std_srvs/srv/Trigger {}
```

| Profile | Weights | Purpose |
|---------|---------|---------|
| `coco80` | `yolo-models/yolo11n.pt` or Ultralytics download | COCO-80 |
| `tank` | `yolo-models/mini_tank_*.pt` / `tank.pt` | mini tank |

See [`../yolo-models/README.md`](../yolo-models/README.md).

---

## Why not `--symlink-install`?

`colcon build --symlink-install` confuses remote IDE indexing (CLion). Lab standard: normal install into `install/`, rebuild after source changes, re-source overlay (via `run/init_…` on the host).
