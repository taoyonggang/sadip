#!/bin/bash

# 退出时如果任意命令失败
set -e

# 设置非交互模式
export DEBIAN_FRONTEND=noninteractive

# 更新并安装必要的工具
apt-get update && apt-get install -y curl gnupg2 lsb-release

# 添加 ROS 2 的 GPG 密钥
curl -sSL https://raw.githubusercontent.com/ros/rosdistro/master/ros.asc | apt-key add -

# 添加 ROS 2 的 APT 仓库
sh -c 'echo "deb [arch=$(dpkg --print-architecture)] http://packages.ros.org/ros2/ubuntu $(lsb_release -cs) main" > /etc/apt/sources.list.d/ros2-latest.list'

# 更新 APT 包索引
apt-get update

# 安装 ROS 2 (以 ROS 2 Humble 为例)
apt-get install -y ros-humble-desktop

# 安装常见的依赖项
apt-get install -y python3-argcomplete python3-colcon-common-extensions

# 设置 ROS 2 环境
source /opt/ros/humble/setup.bash

# 确保每次打开终端时都能加载 ROS 2 环境
echo "source /opt/ros/humble/setup.bash" >> ~/.bashrc

# 确保 rosdep 可以正确初始化和更新
apt-get install -y python3-rosdep

# 初始化 rosdep
rosdep init
rosdep update

echo "ROS 2 安装完成！请重新打开终端或运行 'source ~/.bashrc' 以加载 ROS 2 环境。"
