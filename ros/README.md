# AeroHub ROS workspace

Colcon workspace for ROS 2 packages (`ah_core`, later others).

**Lab baseline:** Jazzy · `RMW_IMPLEMENTATION=rmw_fastrtps_cpp` · `ROS_DOMAIN_ID=42`  
See vault: `projects/aerohub/reference/ros-lab-notes.md`.

## Layout

```
ros/                    # host path: …/aero-hub/ros
  src/
    ah_core/            # Milestone_1 stub node
  build/                # gitignored (created by colcon)
  install/              # gitignored
  log/                  # gitignored
```

---

## Verified lab workflow (Task_12)

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

- Env vars set RMW and domain for every process in the container (including extra `docker exec` shells inherit unless overridden).
- `-p 2222:22` supports remote IDE attach (e.g. CLion) when using this image.
- Host path assumes the stack lives under `~/Documents/projects/pix-eagle-stack/aero-hub`.

### 2. Build `ah_core`

**Do not use `--symlink-install`** for this lab: install-space symlinks confuse remote IDEs such as CLion.

```bash
cd /aero-hub-ros
source /opt/ros/jazzy/setup.bash
# RMW / domain already set via docker --env; re-export if you opened a bare shell:
# export RMW_IMPLEMENTATION=rmw_fastrtps_cpp
# export ROS_DOMAIN_ID=42

colcon build --packages-select ah_core
source install/setup.bash
```

### 3. Run the status publisher

```bash
# same shell (or: source /opt/ros/jazzy/setup.bash && source /aero-hub-ros/install/setup.bash)
ros2 run ah_core ah_core_node
```

### 4. Echo the topic (second shell)

```bash
docker exec -it ros2_dev /bin/bash
source /opt/ros/jazzy/setup.bash
source /aero-hub-ros/install/setup.bash
# domain/RMW from container env if started as above
ros2 topic echo /ah/system/status
```

**Verified:** JSON on `/ah/system/status` at ~5 Hz, e.g.

```text
data: '{"smart_mode_active":false,"tracking_started":false,...,"video_status":"unavailable",...}'
---
```

---

## Why not `--symlink-install`?

`colcon build --symlink-install` puts symlinks into `install/`. That is convenient for rapid edit/rebuild on a pure Linux host, but **breaks or confuses remote IDE indexing** (CLion Compilation Database / remote paths). This lab standard is a normal install layout:

```bash
colcon build --packages-select ah_core
```

Rebuild after source changes; then `source install/setup.bash` again if needed.
