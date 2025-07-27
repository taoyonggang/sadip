#!/bin/bash

# 设置需要安装的 CMake 版本
CMAKE_VERSION="3.28.1"

# 创建临时目录
mkdir -p ~/cmake_install
cd ~/cmake_install

# 下载适用于 ARM64 的 CMake
echo "正在下载 CMake ${CMAKE_VERSION} (ARM64 版本)..."
wget https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}-linux-aarch64.sh

# 添加执行权限
chmod +x cmake-${CMAKE_VERSION}-linux-aarch64.sh

# 执行安装
echo "正在安装 CMake ${CMAKE_VERSION}..."
./cmake-${CMAKE_VERSION}-linux-aarch64.sh --skip-license --prefix=/usr/local

# 清理下载文件
cd ~
rm -rf ~/cmake_install

# 验证安装
echo "CMake 安装完成，当前版本："
cmake --version
