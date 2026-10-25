import os
from datetime import datetime
from uuid import uuid4

from ament_index_python.packages import (
    get_package_prefix,
    get_package_share_directory,
)
from launch import LaunchDescription
from launch.conditions import IfCondition
from launch.actions import (
    DeclareLaunchArgument,
    ExecuteProcess,
    IncludeLaunchDescription, 
    LogInfo,
    OpaqueFunction,
    RegisterEventHandler,
    SetEnvironmentVariable,
)
from launch.event_handlers import OnProcessExit
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import (
    Command,
    EnvironmentVariable,
    FindExecutable,
    LaunchConfiguration,
)
from launch_ros.actions import Node


def _register_bag_playback(context, *, spawn_robot):
    bag_path = context.perform_substitution(LaunchConfiguration("replay_bag"))
    if not bag_path:
        return []

    play_bag = ExecuteProcess(
        cmd=[
            "ros2",
            "bag",
            "play",
            bag_path,
            "--topics",
            "/cmd_vel",
        ],
        output="screen",
    )
    return [
        LogInfo(msg=f"Replaying /cmd_vel from {bag_path}"),
        RegisterEventHandler(
            OnProcessExit(
                target_action=spawn_robot,
                on_exit=[play_bag],
            )
        ),
    ]


def generate_launch_description():
    package_share = get_package_share_directory("gazebo")
    package_prefix = get_package_prefix("gazebo")
    ros_gz_sim_share = get_package_share_directory("ros_gz_sim")

    robot_description = os.path.join(
        package_share, "description", "robot.urdf.xacro"
    )
    world_file = os.path.join(package_share, "worlds", "car_world.sdf")
    install_directory = (
        os.path.dirname(package_prefix)
        if os.path.basename(package_prefix) == "gazebo"
        else package_prefix
    )
    workspace_root = os.path.dirname(install_directory)
    recording_directory = os.path.join(workspace_root, "src", "rosbags")
    os.makedirs(recording_directory, exist_ok=True)
    bag_path = os.path.join(
        recording_directory,
        f"cmd_vel_{datetime.now().strftime('%Y%m%d_%H%M%S_%f')}",
    )
    ignition_partition_name = f"stockcar_{uuid4().hex}"
    ignition_partition = SetEnvironmentVariable(
        name="IGN_PARTITION",
        value=ignition_partition_name,
    )

    gazebo_resource_path = SetEnvironmentVariable(
        name="GZ_SIM_RESOURCE_PATH",
        value=[
            os.path.dirname(package_share),
            os.pathsep,
            EnvironmentVariable("GZ_SIM_RESOURCE_PATH", default_value=""),
        ],
    )

    gazebo = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(ros_gz_sim_share, "launch", "gz_sim.launch.py")
        ),
        launch_arguments={"gz_args": f"-r {world_file}"}.items(),
    )

    description = Node(
        package="robot_state_publisher",
        executable="robot_state_publisher",
        name="description",
        parameters=[
            {
                "robot_description": Command(
                    [FindExecutable(name="xacro"), " ", robot_description]
                ),
                "use_sim_time": True,
            }
        ],
    )

    spawn_robot = Node(
        package="ros_gz_sim",
        executable="create",
        arguments=[
            "-world",
            "car_world",
            "-topic",
            "robot_description",
            "-name",
            "four_wheel_robot",
            "-z",
            "0.0",
        ],
        output="screen",
    )

    bridge = Node(
        package="ros_gz_bridge",
        executable="parameter_bridge",
        name="ros_gz_bridge",
        arguments=[
            "/clock@rosgraph_msgs/msg/Clock[gz.msgs.Clock",
            "/cmd_vel@geometry_msgs/msg/Twist]gz.msgs.Twist",
            "/odom@nav_msgs/msg/Odometry[gz.msgs.Odometry",
            "/tf@tf2_msgs/msg/TFMessage[gz.msgs.Pose_V",
        ],
        output="screen",
    )

    record_bag = ExecuteProcess(
        cmd=[
            "ros2",
            "bag",
            "record",
            "--output",
            bag_path,
            "/cmd_vel",
            "/odom",
        ],
        condition=IfCondition(LaunchConfiguration("record_bag")),
        output="screen",
    )

    return LaunchDescription(
        [
            DeclareLaunchArgument(
                "record_bag",
                default_value="true",
                description="Record /cmd_vel and /odom to a rosbag",
            ),
            DeclareLaunchArgument(
                "replay_bag",
                default_value="",
                description=(
                    "Optional rosbag2 directory to replay after the robot spawns"
                ),
            ),
            OpaqueFunction(
                function=_register_bag_playback,
                kwargs={"spawn_robot": spawn_robot},
            ),
            ignition_partition,
            gazebo_resource_path,
            gazebo,
            description,
            spawn_robot,
            bridge,
            LogInfo(msg=f"Using Ignition partition {ignition_partition_name}"),
            LogInfo(
                msg=f"Recording /cmd_vel and /odom to {bag_path}",
                condition=IfCondition(LaunchConfiguration("record_bag")),
            ),
            record_bag,
        ]
    )
