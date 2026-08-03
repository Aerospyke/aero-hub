# Source this in every terminal that talks to ah_core (bash or zsh).
#
#   cd …/aero-hub/ros
#   source ./init_ah_ros_in_terminal.sh
#
# Why: ah_core reads domain_id=42 from aerohub_settings.ini into *its own process*
# only. Your CLI shell does not inherit that — if ROS_DOMAIN_ID is unset, ros2
# defaults to domain 0 and `ros2 service call` hangs on
# "waiting for service to become available..." while the node is fine on 42.

if [ -n "${ZSH_VERSION:-}" ]; then
  _AH_ROS_DIR="$(cd "$(dirname "${(%):-%x}")" && pwd)"
elif [ -n "${BASH_VERSION:-}" ]; then
  _AH_ROS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
else
  _AH_ROS_DIR="$(cd "$(dirname "$0")" && pwd)"
fi

# Overlay: zsh must use setup.zsh (setup.bash breaks COLCON_CURRENT_PREFIX under zsh).
if [ -n "${ZSH_VERSION:-}" ]; then
  # shellcheck disable=SC1091
  . "${_AH_ROS_DIR}/install/setup.zsh"
else
  # shellcheck disable=SC1091
  . "${_AH_ROS_DIR}/install/setup.bash"
fi

export ROS_DOMAIN_ID="${ROS_DOMAIN_ID:-42}"
export RMW_IMPLEMENTATION="${RMW_IMPLEMENTATION:-rmw_fastrtps_cpp}"

# Optional: same settings file the node uses (camera + ROS keys).
if [ -z "${AERO_HUB_SETTINGS:-}" ] && [ -f "${_AH_ROS_DIR}/../aerohub_settings.ini" ]; then
  export AERO_HUB_SETTINGS="${_AH_ROS_DIR}/../aerohub_settings.ini"
fi

# YOLO weights directory (ah_yolo: coco80 → yolo11n.pt, tank → mini_tank_*.pt / tank.pt)
if [ -z "${AERO_HUB_MODELS:-}" ] && [ -d "${_AH_ROS_DIR}/../models" ]; then
  export AERO_HUB_MODELS="$(cd "${_AH_ROS_DIR}/../models" && pwd)"
fi

echo "[ah_ros] overlay=${_AH_ROS_DIR}/install  ROS_DOMAIN_ID=${ROS_DOMAIN_ID}  RMW=${RMW_IMPLEMENTATION}  MODELS=${AERO_HUB_MODELS:-}"
unset _AH_ROS_DIR
