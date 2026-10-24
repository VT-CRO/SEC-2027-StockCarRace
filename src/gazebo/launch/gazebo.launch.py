import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription, SetEnvironmentVariable
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import Command, EnvironmentVariable, FindExecutable
from launch_ros.actions import Node


def generate_launch_description():
    package_share = get_package_share_directory("gazebo")
    ros_gz_sim_share = get_package_share_directory("ros_gz_sim")

    robot_description = os.path.join(
        package_share, "description", "robot.urdf.xacro"
    )
    world_file = os.path.join(package_share, "worlds", "car_world.sdf")

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

    return LaunchDescription(
        [
            gazebo_resource_path,
            gazebo,
            description,
            spawn_robot,
            bridge,
        ]
    )
