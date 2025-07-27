#!/bin/bash

# 更新系统包列表
echo "Updating package list..."
apt-get update

# 安装 Golang 2.21
echo "Installing Golang 2.21..."
DEBIAN_FRONTEND=noninteractive sudo apt-get install -y golang-1.21-go

# 检查安装结果
#if go version; then
#    echo "Golang 2.21 has been successfully installed."
#else
#    echo "Installation failed."
#    exit 1
#fi

# 配置环境变量和 Go Module
echo "Configuring environment variables and Go module..."

# 添加 Go 的路径到 .profile 文件，以便每次登录时加载
{
    echo 'export PATH=$PATH:/usr/lib/go-1.21/bin/'
    echo 'export GOPATH=$HOME/go'
    echo 'export GOBIN=$GOPATH/bin'
    echo 'export GOMODULE111=on'  # 启用 Go Modules
    echo 'export GOPROXY=https://goproxy.cn,direct'  # 设置国内代理
} >> /etc/profile

# 现在加载这些环境变量
source /etc/profile

# 检查安装结果
if go version; then
    echo "Golang 2.21 has been successfully installed."
else
    echo "Installation failed."
    exit 1
fi


echo "Environment variables and Go module configuration complete."
echo "Installation and configuration have been completed."
