#!/bin/bash

# 退出脚本如果任何命令失败
set -e

# 更新系统
echo "Updating system..."
apt update && apt upgrade -y

# 添加NVIDIA驱动PPA
echo "Adding NVIDIA PPA..."
add-apt-repository ppa:graphics-drivers/ppa -y
apt update

# 安装NVIDIA驱动
echo "Installing NVIDIA drivers..."
apt install -y nvidia-driver-550  

# 重启以加载新驱动（如果需要，取消注释下一行）
# reboot

# 检查NVIDIA驱动是否安装成功
echo "Checking NVIDIA driver installation..."
nvidia-smi

# 下载并安装CUDA 12.4
echo "Installing CUDA 12.4..."
wget https://developer.download.nvidia.com/compute/cuda/12.4.1/local_installers/cuda_12.4.1_550.54.15_linux.run
sh cuda_12.4.1_550.54.15_linux.run --silent --toolkit

# 设置环境变量
echo "Setting up environment variables..."
echo 'export PATH=/usr/local/cuda-12.4/bin:$PATH' >> /etc/profile
echo 'export LD_LIBRARY_PATH=/usr/local/cuda-12.4/lib64:$LD_LIBRARY_PATH' >> /etc/profile

# 重新加载bashrc
source  /etc/profile

# 验证CUDA安装
#echo "Verifying CUDA installation..."
#nvcc --version

echo "Installation completed successfully!"
