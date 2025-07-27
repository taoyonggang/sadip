#!/bin/bash

# 获取脚本所在目录
BASEDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
pushd "${BASEDIR}"

# 清理并创建构建目录
rm -rf build_debug
mkdir -p build_debug

# Conan 安装依赖
conan install . \
    --output-folder=build_debug \
    --build=missing \
    -s build_type=Debug \
	-s tools.system.package_manager:mode=install \
    -s tools.system.package_manager:sudo=False

# 进入构建目录
cd build_debug

# CMake 配置
# 注意：Ubuntu 下不需要指定 Visual Studio 生成器
# 将 Windows 路径替换为 Linux 路径
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake \
    -DCMAKE_PREFIX_PATH="/usr/local;~/local/debug;" \
    -DCMAKE_BUILD_TYPE=Debug

# 构建
cmake --build . --config Debug

# 返回原目录
popd