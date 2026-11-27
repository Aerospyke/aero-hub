# One slam_toolbox per vehicle. Everything is derived from vehicle_ns:
#	in: /<vehicle_ns>/scan	out: /<vehicle_ns>/map
#	frames: <vehicle_ns>/map, <vehicle_ns>/odom, <vehicle_ns>/base_link
#	Example Use: 'ros2 launch ah_mapping ah_slam.launch.py vehicle_namespace:=badger_twin_mark1_0 use_sim_time:=true'

from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, EmitEvent, LogInfo, RegisterEventHandler
from launch.events import matches_action
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import LifecycleNode
from launch_ros.event_handlers import OnStateTransition
from launch_ros.events.lifecycle import ChangeState
from launch_ros.substitutions import FindPackageShare
from lifecycle_msgs.msg import Transition

PACKAGE_NAME_AH_MAPPING = 'ah_mapping'
PARAMETERS_FILE_DIRECTORY = 'config'
PARAMETERS_FILE_NAME_DEFAULT = 'slam_params.yaml'
SLAM_PACKAGE = 'slam_toolbox'
SLAM_EXECUTABLE = 'async_slam_toolbox_node'
NODE_NAME = 'slam_toolbox'
SCAN_TOPIC = 'scan'  # relative (no leading /), so ROS resolves it to /<vehicle_ns>/scan
MAP_TOPIC = 'map'  # relative, so ROS resolves it to /<vehicle_ns>/map
# TF frames aren't namespaced by ROS, we will add the <vehicle_ns> manually in this script
ODOM_FRAME = 'odom'  # becomes <vehicle_ns>/odom
BASE_FRAME = 'base_link'  # becomes <vehicle_ns>/base_link
MAP_FRAME = 'map'  # becomes <vehicle_ns>/map
FRAME_SEPARATOR = '/'

ARGUMENT_VEHICLE_NAMESPACE = 'vehicle_namespace'
ARGUMENT_USE_SIM_TIME = 'use_sim_time'
ARGUMENT_PARAMETERS_FILE_PATH = 'parameters_file_path'
USE_SIM_TIME_DEFAULT_VALUE = 'false'


def generate_launch_description():
    vehicle_namespace = LaunchConfiguration(ARGUMENT_VEHICLE_NAMESPACE)
    use_sim_time = LaunchConfiguration(ARGUMENT_USE_SIM_TIME)
    parameters_file_path = LaunchConfiguration(ARGUMENT_PARAMETERS_FILE_PATH)

    def addNamespacePrefix(frame):
        return [vehicle_namespace, FRAME_SEPARATOR, frame]

    slam = LifecycleNode(
        package=SLAM_PACKAGE,
        executable=SLAM_EXECUTABLE,
        name=NODE_NAME,
        namespace=vehicle_namespace,
        output='screen',
        parameters=[
            parameters_file_path,
            {
                'use_sim_time': use_sim_time,
                'odom_frame': addNamespacePrefix(ODOM_FRAME),
                'map_frame': addNamespacePrefix(MAP_FRAME),
                'base_frame': addNamespacePrefix(BASE_FRAME),
                'scan_topic': SCAN_TOPIC,
                'map_name': MAP_TOPIC,
            },
        ],
    )

    configure = EmitEvent(event=ChangeState(
        lifecycle_node_matcher=matches_action(slam),
        transition_id=Transition.TRANSITION_CONFIGURE))

    activate = RegisterEventHandler(OnStateTransition(
        target_lifecycle_node=slam,
        start_state='configuring',
        goal_state='inactive',
        entities=[
            LogInfo(msg=['slam_toolbox for ', vehicle_namespace, ' is activating']),
            EmitEvent(event=ChangeState(
                lifecycle_node_matcher=matches_action(slam),
                transition_id=Transition.TRANSITION_ACTIVATE)),
        ]))

    return LaunchDescription([
        DeclareLaunchArgument(ARGUMENT_VEHICLE_NAMESPACE,
                              description='Vehicle namespace, e.g. badger_twin_mark1_0'),
        DeclareLaunchArgument(ARGUMENT_USE_SIM_TIME, default_value=USE_SIM_TIME_DEFAULT_VALUE,
                              description='set to true for a simulated vehicle (Gazebo), false for a real vehicle'),
        DeclareLaunchArgument(ARGUMENT_PARAMETERS_FILE_PATH,
                              default_value=PathJoinSubstitution(
                                  [FindPackageShare(PACKAGE_NAME_AH_MAPPING), PARAMETERS_FILE_DIRECTORY,
                                   PARAMETERS_FILE_NAME_DEFAULT]),
                              description='path to the file which contains your slam_toolbox parameters (frames and topics are set here, not in the file)'),
        slam,
        configure,
        activate,
    ])
