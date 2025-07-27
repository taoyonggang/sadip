#!/bin/bash

# 添加 PPA
#add-apt-repository -y ppa:ubuntu-toolchain-r/test
apt update

# 安装 GCC 10 和 G++ 10
apt install -y gcc-10 g++-10

# 设置默认 GCC 和 G++ 版本
update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-10 10 --slave /usr/bin/g++ g++ /usr/bin/g++-10
update-alternatives --install /usr/bin/cc cc /usr/bin/gcc 10
update-alternatives --install /usr/bin/c++ c++ /usr/bin/g++ 10

echo "GCC 10 and G++ 10 installation and default setup complete."
