#!/bin/bash

# 显示使用方法
show_usage() {
    echo "Usage: $0 <build_type> <version>"
    echo "  build_type: 'debug' or 'release'"
    echo "  version: e.g. '1.1.0'"
    echo ""
    echo "Example: $0 debug 1.1.0"
    echo "         $0 release 1.1.0"
}

# 检查参数数量
if [ $# -ne 2 ]; then
    echo "Error: Incorrect number of arguments"
    show_usage
    exit 1
fi

# 从命令行获取构建类型和版本
BUILD_TYPE=$(echo "$1" | tr '[:upper:]' '[:lower:]')  # 转换为小写
ZENOHCPP_VERSION="$2"

# 验证构建类型
if [ "$BUILD_TYPE" != "debug" ] && [ "$BUILD_TYPE" != "release" ]; then
    echo "Error: Invalid build type. Must be 'debug' or 'release'"
    show_usage
    exit 1
fi

# 设置基础路径
BASE_DIR="$HOME/git/zenoh-cpp"
ZENOHC_DIR="$BASE_DIR/zenoh-c"
ZENOH_REPO="https://github.com/eclipse-zenoh/zenoh-cpp.git"

# 基于构建类型设置变量
if [ "$BUILD_TYPE" == "debug" ]; then
    echo "Preparing for DEBUG build of version $ZENOHCPP_VERSION..."
    BUILD_DIR_SUFFIX="debug"
    INSTALL_DIR="$HOME/local/debug"
    CMAKE_BUILD_TYPE="Debug"
    RUSTFLAGS="-g"
    export RUST_BACKTRACE=1
else
    echo "Preparing for RELEASE build of version $ZENOHCPP_VERSION..."
    BUILD_DIR_SUFFIX="release"
    INSTALL_DIR="$HOME/local/release"
    CMAKE_BUILD_TYPE="Release"
    RUSTFLAGS="-C opt-level=3"
fi

# 检查 Rust 是否已安装
# if ! command -v cargo &> /dev/null; then
#     echo "Error: Rust (cargo) is not installed"
#     echo "Please install Rust first using:"
#     echo "curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y"
#     exit 1
# fi

# 确保安装目录存在
mkdir -p "$INSTALL_DIR"

# 确保 git 目录存在
mkdir -p "$HOME/git"

# 删除已有代码并重新克隆
echo "Removing existing code and cloning fresh repository..."
rm -rf "$BASE_DIR"
cd "$HOME/git"
git clone "$ZENOH_REPO"
cd "$BASE_DIR"

# 切换到指定版本
echo "Checking out version $ZENOHCPP_VERSION..."
git checkout $ZENOHCPP_VERSION
git submodule update --init --recursive

# 加载 Rust 环境（如果需要）
if [ -f "$HOME/.cargo/env" ]; then
    source "$HOME/.cargo/env"
fi

# 设置构建目录名
BUILD_DIR="build-$BUILD_DIR_SUFFIX"

# 首先编译 zenoh-c (子模块)
echo "Building zenoh-c $BUILD_TYPE version..."
cd "$ZENOHC_DIR"
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# 设置 Rust 环境变量
export RUSTFLAGS="$RUSTFLAGS"
export CARGO_HOME="$HOME/.cargo"

# 执行CMake构建
cmake .. \
    -DCMAKE_INSTALL_PREFIX="$INSTALL_DIR" \
    -DBUILD_SHARED_LIBS=FALSE \
    -DBUILD_TESTING=ON \
    -DBUILD_EXAMPLES=ON \
    -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE"

cmake --build . --config "$CMAKE_BUILD_TYPE" -- -j$(nproc)
cmake --install .

# 然后编译 zenoh-cpp
echo "Building zenoh-cpp $BUILD_TYPE version..."
cd "$BASE_DIR"
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

cmake .. \
    -DCMAKE_INSTALL_PREFIX="$INSTALL_DIR" \
    -DBUILD_SHARED_LIBS=FALSE \
    -DZENOHCXX_ZENOHC=ON \
    -DZENOHCXX_ZENOHPICO=OFF \
    -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE" \
    -DCMAKE_PREFIX_PATH="$INSTALL_DIR"

# 构建所有目标
cmake --build . --config "$CMAKE_BUILD_TYPE" -- -j$(nproc)
cmake --install . --config "$CMAKE_BUILD_TYPE"

# 构建并运行测试
cmake --build . --target tests --config "$CMAKE_BUILD_TYPE"
ctest -C "$CMAKE_BUILD_TYPE" -V
cmake --build . --target examples --config "$CMAKE_BUILD_TYPE"

echo "$BUILD_TYPE build completed for version $ZENOHCPP_VERSION!"