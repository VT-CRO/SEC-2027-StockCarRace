# Getting Started with SEC 2027 Environment
Install Docker [from here](https://docs.docker.com/desktop/setup/install/windows-install/) if on Windows.

Or do the following
```
sudo apt-get update && sudo apt-get upgrade -y

sudo apt-get install docker.io
```

Run Docker by opening Docker Desktop on Windows or using ```sudo systemctl start docker``` on Linux.

Clone the SEC-2027-StockCarRace repository in a directory of your choosing.
```
git clone https://github.com/VT-CRO/SEC-2027-StockCarRace.git
```

CD into the repository, and switch to the enviroment-setup branch.
```
git switch enviroment-setup
```

Now, either use the README file or continue using this one.

## VT CRO SoutheastCon Jetson Environment Setup

This is a development environment for creating ROS applications. This environment includes GUI features for debugging and developing such as:
- Gazebo
- Rviz

# Prereqs 

If on a Windows computer install WSL Ubuntu before continuing. 

IMPORTANT: The version of WSL Ubuntu has to be 22.04.05. This is because the version of ROS2 we use (humble) runs on this version. The installation link can be found here:

https://apps.microsoft.com/detail/9PN20MSR04DW?hl=en-us&gl=US&ocid=pdpshare

The following instructions should be ran on the Ubuntu terminal / the WSL enviroment, NOT powershell.

Before trying to use this workspace make sure you have the nvidia container toolkit installed and docker installed on your system.

To install the nvidia container toolkit:
```
curl -fsSL https://nvidia.github.io/libnvidia-container/gpgkey | sudo gpg --dearmor -o /usr/share/keyrings/nvidia-container-toolkit-keyring.gpg \
  && curl -s -L https://nvidia.github.io/libnvidia-container/stable/deb/nvidia-container-toolkit.list | \
    sed 's#deb https://#deb [signed-by=/usr/share/keyrings/nvidia-container-toolkit-keyring.gpg] https://#g' | \
    sudo tee /etc/apt/sources.list.d/nvidia-container-toolkit.list

sudo apt-get update

sudo apt-get install -y nvidia-container-toolkit
```


# How to Build

Prerequisites:
- Docker Desktop
- WSL (if windows)

Note: The jetson has an ARM processor, so if you're trying to set up this environment on the jetson, you will need to use the Dockerfile in the balena directory.

Build the image using

```
docker build -t vt-cro/ros-env .
```
This process will take a long time. That is ok.

IMPORTANT: Before running the container, make sure to run the command `xhost +local:` on your host system (if you're on a Linux system or a Linux Virtual Enviroment (WSL)) BEFORE running the launch script. This will grant the container to access your host's X server, allowing you to run graphical applications such as rviz or gazebo. No further setup is needed if running with a Windows or Mac host. 

Finally, you can run the container by running `launch.sh` shell script if you have a gpu and the `launch_no_gpu.sh` if no gpu is present in your system
Place your workspace in `./src/` and you will be able to access it within the container at `/home/ros/ws/`.

You may need to give permission to execute the script:
```
chmod +x scriptname (either launch.sh or launch_no_gpu.sh)
```

To execute the script:
```
sudo ./scriptname (either launch.sh or launch_no_gpu.sh)
```

# How to Build Workspace
Navigate to the workspace (`/home/ros/ws`) and run the following commands to install all required dependencies for the ROS workspace:

```
sudo apt update
source install/setup.bash
rosdep --install-from-paths src -y --ignore-src
```

If the above doesn't work, try
```
sudo apt update
source /opt/ros/humble/setup.bash
rosdep update
rosdep install --from-paths src --ignore-src -r -y
```

If there is no ```/src``` directory, make one. Then run those commands again.

Clone the SEC-2027-StockCarRace repository into ```/ws```
```
git clone https://github.com/VT-CRO/SEC-2027-StockCarRace.git
```

Then navigate to the SEC-2027-StockCarRace directory you just cloned and build the workspace. To do so, you must ensure to build the packages in the following order. The commands are as follows:
```
colcon build --packages-select btcpp_ros2_interfaces
colcon build --packages-select behaviortree_ros2
colcon build --packages-select apriltag_ros
colcon build
```
This will take a while.

Now source the workspace again, and is is ready to use.

# To open / close the image
To check what containers are currently running / not running:

```
docker ps -a
```

To start a container:
```
docker start "image_name"
```

To enter a container:
```
docker exec -it "image_name" /bin/bash
```

To stop a container:
```
docker stop "image_name"
```

# Tips for Troubleshooting

When using `colon build' if you get a permissions error, do the following:
```
cd 

chmod 777 ws/
```

Then try again.