#!/bin/bash

# 添加 PPA
#add-apt-repository -y ppa:ubuntu-toolchain-r/test
apt update

# 安装 GCC 10 和 G++ 10
apt install -y gcc-9 g++-9

# 设置默认 GCC 和 G++ 版本
update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-9 9 --slave /usr/bin/g++ g++ /usr/bin/g++-9
update-alternatives --install /usr/bin/cc cc /usr/bin/gcc 9
update-alternatives --install /usr/bin/c++ c++ /usr/bin/g++ 9

echo "GCC 9 and G++ 9 installation and default setup complete."
