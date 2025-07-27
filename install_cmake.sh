#!/bin/bash

# 更新包列表并安装必要的软件包
apt-get update -y
apt-get install -y ca-certificates gpg wget

# 添加Kitware的GPG密钥
wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc | gpg --dearmor - | tee /usr/share/keyrings/kitware-archive-keyring.gpg >/dev/null

# 添加Kitware APT仓库
echo 'deb [signed-by=/usr/share/keyrings/kitware-archive-keyring.gpg] https://apt.kitware.com/ubuntu/ focal main' | tee /etc/apt/sources.list.d/kitware.list >/dev/null

# 更新APT包列表
apt-get update -y

# 安装Kitware的密钥包
apt-get install -y kitware-archive-keyring

# 安装CMake
apt-get install -y cmake

# 验证安装
cmake --version
