#!/bin/bash

# 更新系统包列表
sudo apt-get update

# 安装 dirmngr，这是管理和下载公钥所需的
sudo apt-get install -y dirmngr gnupg

# 确保 GPG 目录存在并设置正确的权限
sudo mkdir -p /root/.gnupg
sudo chmod 700 /root/.gnupg

# 下载 NVIDIA CUDA 存储库的 GPG 密钥
wget http://developer.download.nvidia.com/compute/cuda/repos/ubuntu2004/x86_64/7fa2af80.pub -O /tmp/cuda-signing-key.pub

# 导入下载的密钥
sudo gpg --no-default-keyring --keyring /usr/share/keyrings/nvidia-cuda-archive-keyring.gpg --import /tmp/cuda-signing-key.pub

# 将 CUDA 存储库添加到系统的 APT 源
echo "deb [signed-by=/usr/share/keyrings/nvidia-cuda-archive-keyring.gpg] http://developer.download.nvidia.com/compute/cuda/repos/ubuntu2004/x86_64 /" | sudo tee /etc/apt/sources.list.d/cuda.list

# 更新 APT 包列表以包括新的 CUDA 存储库
sudo apt-get update

# 安装 CUDA（这将安装最新版本的 CUDA）
sudo apt-get install -y cuda

sudo apt-get install -y nvidia-driver-550
