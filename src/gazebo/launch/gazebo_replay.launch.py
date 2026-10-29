import os
from glob import glob

from ament_index_python.packages import (
    get_package_prefix,
    get_package_share_directory,
)
from launch import LaunchDescription
from launch.actions import (
    DeclareLaunchArgument,
    ExecuteProcess,
    IncludeLaunchDescription,
    LogInfo,
    RegisterEventHandler,
)
from launch.event_handlers import OnProcessExit
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution


def generate_launch_description():
    package_share = get_package_share_directory("gazebo")
    package_prefix = get_package_prefix("gazebo")
    install_directory = (
        os.path.dirname(package_prefix)
        if os.path.basename(package_prefix) == "gazebo"
        else package_prefix
    )

    # Determine the path to rosbag to replay (defaults to most recent rosbag generated if path arg is not passed)

    workspace_root = os.path.dirname(install_directory)

    bag_directories = [
        path
        for path in glob(
            os.path.join(workspace_root, "src", "rosbags", "cmd_vel_*")
        )
        if os.path.isdir(path)
        and os.path.isfile(os.path.join(path, "metadata.yaml"))
    ]
    if not bag_directories:
        raise RuntimeError(
            "No completed cmd_vel bag found under src/rosbags. "
            "Record one with gazebo.launch.py or pass "
            "path:=<workspace-relative path>."
        )
    latest_bag_path = os.path.relpath(
        max(bag_directories, key=os.path.getmtime), workspace_root
    )


    # Launches gazebo.launch.py, ros2 bag
    
    gazebo_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(package_share, "launch", "gazebo.launch.py")
        ),
        launch_arguments={"record": "false"}.items(),
    )
    wait_for_odom = ExecuteProcess(
        cmd=["ros2", "topic", "echo", "/odom", "--once"],
        output="log",
    )
    bag_path = PathJoinSubstitution(
        [workspace_root, LaunchConfiguration("path")]
    )
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

    return LaunchDescription(
        [
            DeclareLaunchArgument(
                "path",
                default_value=latest_bag_path,
                description=(
                    "Rosbag2 directory relative to the workspace root; "
                    "defaults to the newest completed cmd_vel bag in src/rosbags"
                ),
            ),
            LogInfo(msg=["Replaying /cmd_vel from ", bag_path]),
            gazebo_launch,
            RegisterEventHandler(
                OnProcessExit(
                    target_action=wait_for_odom,
                    on_exit=[play_bag],
                )
            ),
            wait_for_odom,
        ]
    )
