# AeroHub ROS workspace

Colcon workspace for ROS 2 packages (`ah_core`, later others).

**Lab baseline:** Jazzy · `RMW_IMPLEMENTATION=rmw_fastrtps_cpp` · `ROS_DOMAIN_ID=42`  
See vault: `projects/aerohub/reference/ros-lab-notes.md`.

## Layout

```
ros/                    # host path: …/aero-hub/ros
  src/
    ah_msgs/            # StartTracking, ListCameras, SelectCamera, …
    ah_core/            # status + video + track + camera list/select
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

#### macOS / RoboStack host (not Docker)

```bash
conda activate ros_env
cd ~/Documents/projects/pix-eagle-stack/aero-hub/ros
export ROS_DOMAIN_ID=42
export RMW_IMPLEMENTATION=rmw_fastrtps_cpp
colcon build --packages-select ah_msgs ah_core

# IMPORTANT — every terminal (node *and* service-call CLI):
source ./init_ah_ros_in_terminal.sh
# That sets overlay + ROS_DOMAIN_ID=42 + RMW. Bare `source install/setup.zsh`
# alone is not enough if domain is unset (CLI stays on domain 0 → service call hangs).
```

**Domain footgun:** `ah_core` applies `[ROS] domain_id=42` from `aerohub_settings.ini` **only inside the node process**. Terminal 2 does not get that automatically. If Terminal 2 has no `ROS_DOMAIN_ID`, `ros2 service call` hangs on `waiting for service to become available...` while Terminal 1 looks healthy.

```bash
# Terminal 2 must also:
cd …/aero-hub/ros
source ./init_ah_ros_in_terminal.sh
echo $ROS_DOMAIN_ID   # must print 42
ros2 service list | grep camera
ros2 service call /ah/camera/list ah_msgs/srv/ListCameras "{refresh: true}"
```

**`install_name_tool` / code-signature lines:** noise from conda’s tools. If colcon prints `Finished <<< ah_core` / `Finished <<< ah_msgs`, the packages installed. Real failure = missing binary or non-zero exit.

**`AMENT_PREFIX_PATH … ah_core doesn't exist`:** you deleted `install/ah_core` while an old `source install/setup.*` was still in that shell. Harmless for the rebuild; re-source after build.

If the install looks stale:

```bash
rm -rf build/ah_core install/ah_core
colcon build --packages-select ah_msgs ah_core
source install/setup.zsh   # or setup.bash under bash
```

### 3. Run the stub node

From workspace root `ros/` (after overlay sourced):

```bash
export ROS_DOMAIN_ID=42
ros2 run ah_core ah_core_node
# wait for:  ah_core ready: …
```

| Topic / service | Type | Notes |
|-----------------|------|--------|
| `ah/system/status` → `/[ns/]ah/system/status` | `std_msgs/String` (JSON) | ~10 Hz; relative name + node namespace |
| `ah/video/compressed` | `sensor_msgs/CompressedImage` | Synthetic JPEG ~10 Hz (live cam = Task_31) |
| `ah/tracking/start` | `ah_msgs/srv/StartTracking` | normalized tracking bounding box [0,1] |
| `ah/tracking/stop` / `cancel` | `std_srvs/Trigger` | stop vs hard reset |
| `ah/camera/list` | `ah_msgs/srv/ListCameras` | Enumerate devices (Task_30); no hard-coded IDs |
| `ah/camera/select` | `ah_msgs/srv/SelectCamera` | Select synthetic or camera; persists to params + `[Camera]` INI |

**Namespace (multi-drone):** set `[ROS] namespace=` in `aerohub_settings.ini` (cwd) or `AERO_HUB_ROS_NAMESPACE`. Empty = root (`/ah/...`). Example `uav1` → `/uav1/ah/...`. Dashboard and core must match.

**Camera (Task_30):** selection is owned by `ah_core`. Params: `video.source`, `camera.device_id`, `camera.device_path`, `camera.backend`. Prefer `device_path` (`index:N`, `/dev/videoN`, or `synthetic`) over bare indices so USB/OBS reordering does not require code changes. Live frames from the selected camera land in Task_31; until then video stays synthetic while selection is still valid in status/params.

### 4. Verify (second shell)

```bash
docker exec -it ros2_dev /bin/bash
source /opt/ros/jazzy/setup.bash
source /aero-hub-ros/install/setup.bash

# root namespace (default); if namespace=uav1 use /uav1/ah/...
ros2 topic echo /ah/system/status
ros2 topic echo /ah/video/compressed --no-arr

# Task_14 — track services
ros2 service list | grep tracking
ros2 service call /ah/tracking/start ah_msgs/srv/StartTracking \
  "{x: 0.1, y: 0.1, width: 0.2, height: 0.2}"
ros2 service call /ah/tracking/stop std_srvs/srv/Trigger {}
ros2 service call /ah/tracking/cancel std_srvs/srv/Trigger {}

# Task_30 — camera list / select (no hard-coded device IDs)
ros2 service list | grep camera
ros2 service call /ah/camera/list ah_msgs/srv/ListCameras "{refresh: true}"
ros2 service call /ah/camera/select ah_msgs/srv/SelectCamera \
  "{video_source: 'synthetic', device_id: -1, device_path: 'synthetic', backend: ''}"
# After list shows Camera 0 path index:0:
ros2 service call /ah/camera/select ah_msgs/srv/SelectCamera \
  "{video_source: 'camera', device_id: 0, device_path: 'index:0', backend: ''}"
ros2 param get /ah_core video.source
ros2 param get /ah_core camera.device_path
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
**Task_30:** camera list/select services + params + optional `[Camera]` in settings INI.

---

## Why not `--symlink-install`?

`colcon build --symlink-install` puts symlinks into `install/`. Convenient on pure Linux hosts, but **confuses remote IDE indexing** (CLion). Lab standard:

```bash
colcon build --packages-select ah_msgs ah_core
```

Rebuild after source changes; re-`source install/setup.bash` if needed.
