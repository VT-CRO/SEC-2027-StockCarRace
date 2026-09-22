FROM osrf/ros:humble-desktop

RUN apt-get update && apt-get install -y\
  vim \
  && rm -rf /var/lib/apt/lists/*

ARG USERNAME=ros
ARG USER_UID=1000
ARG USER_GID=${USER_UID}

RUN groupadd --gid $USER_GID $USERNAME \
  && useradd -s /bin/bash --uid $USER_UID --gid $USER_GID -m $USERNAME \
  && mkdir /home/$USERNAME/.config && chown $USER_UID:$USER_GID /home/$USERNAME/.config


RUN apt-get update  \
  && apt-get upgrade -y \
  && apt-get install -y wget \
  && apt-get install -y lsb-release gnupg 

# Install most ROS dependencies
RUN apt-get update \
  && apt-get install -y ros-${ROS_DISTRO}-gazebo-ros-pkgs \
    ros-${ROS_DISTRO}-xacro \
    ros-${ROS_DISTRO}-joint-state-publisher-gui \
    ros-${ROS_DISTRO}-image-transport-plugins \
    ros-${ROS_DISTRO}-rqt-image-view \
    ros-${ROS_DISTRO}-ros2-control \
    ros-${ROS_DISTRO}-ros2-controllers \
    ros-${ROS_DISTRO}-gazebo-ros2-control \
    ros-${ROS_DISTRO}-rviz2

RUN apt-get update && apt-get install -y \
  libzmq3-dev \
  libboost-dev \
  libboost-system-dev \
  libboost-filesystem-dev \
  libboost-thread-dev \
  libprotobuf-dev \
  protobuf-compiler \
  libmsgsl-dev\
  libgtest-dev \
  cmake

# Install Navigation2
RUN apt update && apt install \
    ros-${ROS_DISTRO}-navigation2 \
    ros-${ROS_DISTRO}-nav2-bringup \

# Install Behavior Tree repository
WORKDIR /tmp
RUN git clone https://github.com/BehaviorTree/BehaviorTree.CPP.git && \
  cd BehaviorTree.CPP && \
  mkdir build && cd build && \
  cmake .. && \
  make && make install

# Install ROS2 Apriltag repository
RUN mkdir -p /opt/underlay_ws/src && \
  git clone https://github.com/christianrauch/apriltag_ros.git \
    /opt/underlay_ws/src/apriltag_ros && \
  . /opt/ros/${ROS_DISTRO}/setup.sh && \
  apt-get update && \
  rosdep update --rosdistro ${ROS_DISTRO} && \
  rosdep install --from-paths /opt/underlay_ws/src --ignore-src -r -y && \
  cd /opt/underlay_ws && \
  colcon build --merge-install --cmake-args -DCMAKE_BUILD_TYPE=Release && \
  rm -rf /opt/underlay_ws/build /opt/underlay_ws/log
  

RUN apt-get update \
  && apt-get install -y sudo \
  && echo $USERNAME ALL=\(root\) NOPASSWD:ALL > /etc/sudoers.d/$USERNAME \
  && chmod 0440 /etc/sudoers.d/$USERNAME \
  && rm -rf /var/lib/apt/lists/*

COPY entrypoint.sh /entrypoint.sh

WORKDIR /home/ros/ws/

ENTRYPOINT ["/bin/bash", "/entrypoint.sh"]

RUN echo "source /opt/ros/humble/setup.bash" >> /home/ros/.bashrc
RUN echo "source /usr/share/gazebo/setup.bash" >> /home/ros/.bashrc
RUN echo "source /opt/underlay_ws/install/setup.bash" >> /home/ros/.bashrc

CMD ["bash"]

USER $USERNAME