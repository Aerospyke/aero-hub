# AeroHub ROS workspace

Colcon workspace for ROS 2 packages (`ah_core`, later others).

**Lab baseline:** Jazzy · `RMW_IMPLEMENTATION=rmw_fastrtps_cpp` · `ROS_DOMAIN_ID=42`  
See vault: `projects/aerohub/reference/ros-lab-notes.md`.

## Layout

```
ros/                    # host path: …/aero-hub/ros
  src/
    ah_msgs/            # StartTracking.srv (and later messages)
    ah_core/            # Milestone_1 stub (status + video + track services)
  build/                # gitignored (created by colcon)
  install/              # gitignored
  log/                  # gitignored
```

---

## Verified lab workflow

### 1. Start container

Mount **only** the ROS workspace (this directory) at `/aero-hub-ros`:

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

Notes:

- Env vars set RMW and domain for every process in the container (including extra `docker exec` shells).
- `-p 2222:22` supports remote IDE attach (e.g. CLion).
- Host path assumes the stack lives under `~/Documents/projects/pix-eagle-stack/aero-hub`.

### 2. Build `ah_msgs` + `ah_core`

**Do not use `--symlink-install`** for this lab: install-space symlinks confuse remote IDEs such as CLion.

Requires OpenCV (`libopencv-dev` / desktop image usually has it) for JPEG encode.  
Build **`ah_msgs` with `ah_core`** (`ah_core` depends on `StartTracking.srv`).

```bash
cd /aero-hub-ros
source /opt/ros/jazzy/setup.bash
# RMW / domain already set via docker --env; re-export if needed:
# export RMW_IMPLEMENTATION=rmw_fastrtps_cpp
# export ROS_DOMAIN_ID=42

colcon build --packages-select ah_msgs ah_core
source install/setup.bash
```

### 3. Run the stub node

```bash
ros2 run ah_core ah_core_node
```

| Topic / service | Type | Notes |
|-----------------|------|--------|
| `/ah/system/status` | `std_msgs/String` (JSON) | ~10 Hz; `tracking_started` flips with services |
| `/ah/video/compressed` | `sensor_msgs/CompressedImage` | Synthetic JPEG (color bars + bouncing box) ~10 Hz |
| `/ah/tracking/start` | `ah_msgs/srv/StartTracking` | normalized tracking bounding box [0,1]; sets tracking on |
| `/ah/tracking/stop` | `std_srvs/Trigger` | tracking off only |
| `/ah/tracking/cancel` | `std_srvs/Trigger` | hard reset (tracking + segmentation flag) |

### 4. Verify (second shell)

```bash
docker exec -it ros2_dev /bin/bash
source /opt/ros/jazzy/setup.bash
source /aero-hub-ros/install/setup.bash

ros2 topic echo /ah/system/status
ros2 topic echo /ah/video/compressed --no-arr

# Task_14 — track services
ros2 service list | grep tracking
ros2 service call /ah/tracking/start ah_msgs/srv/StartTracking \
  "{x: 0.1, y: 0.1, width: 0.2, height: 0.2}"
# → status JSON tracking_started: true
ros2 service call /ah/tracking/stop std_srvs/srv/Trigger {}
ros2 service call /ah/tracking/cancel std_srvs/srv/Trigger {}
```

### Optional: view frames with `rqt_image_view` (XQuartz on Mac host)

Host:

```bash
open -a XQuartz
xhost +localhost
```

Container (with `ah_core_node` running and `DISPLAY=host.docker.internal:0`):

```bash
source /opt/ros/jazzy/setup.bash
source /aero-hub-ros/install/setup.bash
ros2 run rqt_image_view rqt_image_view
```

In the UI, select **`/ah/video/compressed`** (not a `…/raw` name).

**Do not** invent `/ah/video/raw` as a topic name for `image_transport`: names ending in `/raw` or `/compressed` are treated as *transport suffixes* on a *base* topic (e.g. base `/ah/video` + transport `compressed` → `/ah/video/compressed`). Our stub **publishes `sensor_msgs/CompressedImage` directly** on `/ah/video/compressed`, so subscribe to that full topic in `rqt_image_view`.

**Task_12 verified:** JSON status echo.  
**Task_13 verified:** compressed JPEG stream; viewable via `rqt_image_view` on `/ah/video/compressed`.  
**Task_14:** track services on stub — rebuild `ah_msgs` + `ah_core`, then service call examples above.

---

## Why not `--symlink-install`?

`colcon build --symlink-install` puts symlinks into `install/`. Convenient on pure Linux hosts, but **confuses remote IDE indexing** (CLion). Lab standard:

```bash
colcon build --packages-select ah_msgs ah_core
```

Rebuild after source changes; re-`source install/setup.bash` if needed.
