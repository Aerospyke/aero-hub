# AeroHub (code)

Modular companion stack: **DashboardApp** (Qt/QML) + **FlightInstruments** module + **ROS** packages under `ros/`.

## Documentation

Primary docs live in the Obsidian vault:

```
~/Documents/obsidian-vault/projects/aerohub/
```

### ROS lab baseline

| Setting | Value |
|---------|--------|
| Distro | ROS 2 **Jazzy** |
| RMW | `rmw_fastrtps_cpp` |
| Domain | `ROS_DOMAIN_ID=42` |

Full notes: vault `reference/ros-lab-notes.md` · package steps: `ros/README.md`.

| Role | Runtime |
|------|---------|
| **DashboardApp** + `rclcpp` (Task_15+) | macOS **RoboStack** + **plain CMake/Qt** (not colcon); `conda activate ros_env` |
| **`ah_core`** | Docker Jazzy + **colcon** under `ros/`, domain **42**, `rmw_fastrtps_cpp` |

All AeroHub IPC stays on ROS for now. Optional WS/HTTP gateway node is later (parking lot).

**CLion + RoboStack tip:** if Compilation Database fails on conda’s `arm64-apple-darwin*-clang`, build with system compilers for the DB only:

```bash
conda activate ros_env
colcon build --cmake-args \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
  -DCMAKE_C_COMPILER=/usr/bin/clang \
  -DCMAKE_CXX_COMPILER=/usr/bin/clang++
```

See vault `reference/ros-lab-notes.md`.

## Build

### Dashboard (Qt)

Use your usual CMake/Conan flow for `DashboardApp` (see `CMakeLists.txt` / `QtAppSetup.cmake`).

### ROS (`ah_core` stub) — verified lab steps

```bash
# Host: start container (mount only ros/ workspace)
docker run -it --rm --name ros2_dev \
  --cap-add=sys_ptrace \
  --env="DISPLAY=host.docker.internal:0" \
  --env="RMW_IMPLEMENTATION=rmw_fastrtps_cpp" \
  --env="ROS_DOMAIN_ID=42" \
  --volume /tmp/.X11-unix:/tmp/.X11-unix:rw \
  --volume ~/Documents/projects/pix-eagle-stack/aero-hub/ros:/aero-hub-ros \
  -p 2222:22 \
  conorco/ros:jazzy-desktop-full-with-clion

# Inside container
cd /aero-hub-ros
source /opt/ros/jazzy/setup.bash
colcon build --packages-select ah_core    # no --symlink-install (CLion-friendly)
source install/setup.bash
ros2 run ah_core ah_core_node
# publishes /ah/system/status + synthetic /ah/video/compressed

# Second shell
docker exec -it ros2_dev /bin/bash
source /opt/ros/jazzy/setup.bash
source /aero-hub-ros/install/setup.bash
ros2 topic echo /ah/system/status
ros2 topic echo /ah/video/compressed --no-arr
```
