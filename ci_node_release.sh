#!/bin/bash

# 获取脚本所在目录
BASEDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
pushd "${BASEDIR}"

# 定义安装目录 - 您可以根据需要修改此变量
INSTALL_DIR="/root/sadip"  

# 清理并创建构建目录
rm -rf build_release
mkdir -p build_release

# Conan 安装依赖
conan install . \
    --output-folder=build_release \
    --build=missing \
    -s build_type=Release \
    -s tools.system.package_manager:mode=install \
    -s tools.system.package_manager:sudo=False

# 进入构建目录
cd build_release

# CMake 配置，添加安装路径
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake \
    -DCMAKE_PREFIX_PATH="/usr/local/fastrtps-2.11.3;/usr/local;$HOME/local/release" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="${INSTALL_DIR}"

# 构建
cmake --build . --config Release

# 安装到指定目录
# 如果安装目录需要sudo权限，请使用sudo
if [[ "${INSTALL_DIR}" == /usr/* ]] || [[ "${INSTALL_DIR}" == /opt/* ]]; then
    echo "Installing to system directory, sudo may be required..."
    sudo cmake --install . --config Release
else
    echo "Installing to user directory..."
    # 确保安装目录存在
    mkdir -p "${INSTALL_DIR}"
    cmake --install . --config Release
fi

echo "Installation completed to ${INSTALL_DIR}"

# 返回原始目录
popd