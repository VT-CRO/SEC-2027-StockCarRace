import os
from glob import glob

from ament_index_python.packages import (
    get_package_prefix,
    get_package_share_directory,
)
from launch import LaunchDescription
from launch.actions import (
    DeclareLaunchArgument,
    IncludeLaunchDescription,
    OpaqueFunction,
)
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration


def _include_gazebo_with_bag(context, *, package_share, workspace_root):
    bag_path = context.perform_substitution(LaunchConfiguration("bag_path"))

    if not bag_path:
        bag_directories = glob(
            os.path.join(workspace_root, "src", "rosbags", "cmd_vel_*")
        )
        bag_directories = [
            path
            for path in bag_directories
            if os.path.isdir(path)
            and os.path.isfile(os.path.join(path, "metadata.yaml"))
        ]
        if not bag_directories:
            raise RuntimeError(
                "No completed cmd_vel bag found under src/rosbags. "
                "Record one with gazebo.launch.py or pass "
                "bag_path:=<relative path>."
            )
        bag_path = os.path.relpath(
            max(bag_directories, key=os.path.getmtime), workspace_root
        )

    if os.path.isabs(bag_path):
        raise RuntimeError(
            "bag_path must be relative to the SEC-2027-StockCarRace workspace."
        )

    resolved_bag_path = os.path.realpath(
        os.path.join(workspace_root, bag_path)
    )
    if os.path.commonpath([workspace_root, resolved_bag_path]) != workspace_root:
        raise RuntimeError("bag_path must resolve inside the workspace.")
    if not os.path.isfile(os.path.join(resolved_bag_path, "metadata.yaml")):
        raise RuntimeError(
            f"No rosbag2 metadata.yaml found at workspace-relative path "
            f"'{bag_path}'."
        )

    gazebo_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(package_share, "launch", "gazebo.launch.py")
        ),
        launch_arguments={
            "record_bag": "false",
            "replay_bag": resolved_bag_path,
        }.items(),
    )
    return [gazebo_launch]


def generate_launch_description():
    package_share = get_package_share_directory("gazebo")
    package_prefix = get_package_prefix("gazebo")
    install_directory = (
        os.path.dirname(package_prefix)
        if os.path.basename(package_prefix) == "gazebo"
        else package_prefix
    )
    workspace_root = os.path.dirname(install_directory)

    return LaunchDescription(
        [
            DeclareLaunchArgument(
                "bag_path",
                default_value="",
                description=(
                    "Rosbag2 directory relative to the workspace root; "
                    "defaults to the newest completed cmd_vel bag in src/rosbags"
                ),
            ),
            OpaqueFunction(
                function=_include_gazebo_with_bag,
                kwargs={
                    "package_share": package_share,
                    "workspace_root": workspace_root,
                },
            ),
        ]
    )
