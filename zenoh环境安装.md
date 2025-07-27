# 当前 shell 的代理
$env:HTTP_PROXY="http://10.10.1.248:10809"
$env:HTTPS_PROXY="http://10.10.1.248:10809"

rem 当前 shell 的代理
set http_proxy=http://10.10.1.248:10809
set https_proxy=http://10.10.1.248:10809

#linux
export http_proxy=http://10.10.1.248:10809
export https_proxy=http://10.10.1.248:10809

sudo snap set system proxy.http="http://10.10.1.248:10809"
sudo snap set system proxy.https="http://10.10.1.248:10809"

由于zeonh-c和zenoh-cpp版本并非一一对应，需要按照zenoh-cpp 子模块版本进行编译

1. 下载源码
# 在 zenoh-cpp 目录
cd C:/git
git clone https://github.com/eclipse-zenoh/zenoh-cpp
cd C:/git/zenoh-cpp
git fetch --all --tags
git checkout 1.2.1
```

2. windows 安装脚本
```
# 版本信息
$ZENOHCPP_VERSION = "1.2.1"

# 设置基础路径
$BASE_DIR = "C:\git\zenoh-cpp"
$ZENOHC_DIR = "$BASE_DIR\zenoh-c"
$INSTALL_DIR_DEBUG = "C:\local\debug"
$INSTALL_DIR_RELEASE = "C:\local\release"

# 检查 Rust 是否已安装
if (-not (Get-Command cargo -ErrorAction SilentlyContinue)) {
    Write-Error "Error: Rust (cargo) is not installed"
    Write-Host "Please install Rust first from: https://rustup.rs/"
    exit 1
}

# 确保安装目录存在
New-Item -ItemType Directory -Force -Path $INSTALL_DIR_DEBUG | Out-Null
New-Item -ItemType Directory -Force -Path $INSTALL_DIR_RELEASE | Out-Null

# 更新代码和切换到指定版本
Set-Location $BASE_DIR
git fetch --all --tags
git checkout $ZENOHCPP_VERSION
git submodule update --init --recursive

# 首先编译 zenoh-c (子模块)
Write-Host "Building zenoh-c..."
Set-Location $ZENOHC_DIR

# Debug 版本 zenoh-c
Write-Host "Building zenoh-c Debug version..."
Remove-Item -Path "build-debug" -Recurse -Force -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force -Path "build-debug" | Out-Null
Set-Location "build-debug"

# 设置 Rust 环境变量
$env:RUSTFLAGS = "-g"
$env:RUST_BACKTRACE = "1"

cmake .. `
    -G "Visual Studio 17 2022" -A x64 `
    -DCMAKE_INSTALL_PREFIX="$INSTALL_DIR_DEBUG" `
    -DBUILD_SHARED_LIBS=FALSE `
    -DBUILD_TESTING=ON `
    -DBUILD_EXAMPLES=ON `
    -DCMAKE_BUILD_TYPE=Debug

cmake --build . --config Debug
cmake --install . --config Debug

# Release 版本 zenoh-c
Write-Host "Building zenoh-c Release version..."
Set-Location $ZENOHC_DIR
Remove-Item -Path "build-release" -Recurse -Force -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force -Path "build-release" | Out-Null
Set-Location "build-release"

# 设置 Rust 环境变量
$env:RUSTFLAGS = "-C opt-level=3"

cmake .. `
    -G "Visual Studio 17 2022" -A x64 `
    -DCMAKE_INSTALL_PREFIX="$INSTALL_DIR_RELEASE" `
    -DBUILD_SHARED_LIBS=FALSE `
    -DBUILD_TESTING=ON `
    -DBUILD_EXAMPLES=ON `
    -DCMAKE_BUILD_TYPE=Release

cmake --build . --config Release
cmake --install . --config Release

# 然后编译 zenoh-cpp
Write-Host "Building zenoh-cpp..."

# Debug 版本 zenoh-cpp
Write-Host "Building zenoh-cpp Debug version..."
Set-Location $BASE_DIR
Remove-Item -Path "build-debug" -Recurse -Force -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force -Path "build-debug" | Out-Null
Set-Location "build-debug"

cmake .. `
    -G "Visual Studio 17 2022" -A x64 `
    -DCMAKE_INSTALL_PREFIX="$INSTALL_DIR_DEBUG" `
    -DBUILD_SHARED_LIBS=FALSE `
    -DZENOHCXX_ZENOHC=ON `
    -DZENOHCXX_ZENOHPICO=OFF `
    -DCMAKE_BUILD_TYPE=Debug `
    -DCMAKE_PREFIX_PATH="$INSTALL_DIR_DEBUG"

# 构建所有目标
cmake --build . --config Debug
cmake --install . --config Debug

# 构建并运行测试
cmake --build . --target tests --config Debug
ctest -C Debug -V
cmake --build . --target examples --config Debug

# Release 版本 zenoh-cpp
Write-Host "Building zenoh-cpp Release version..."
Set-Location $BASE_DIR
Remove-Item -Path "build-release" -Recurse -Force -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force -Path "build-release" | Out-Null
Set-Location "build-release"

cmake .. `
    -G "Visual Studio 17 2022" -A x64 `
    -DCMAKE_INSTALL_PREFIX="$INSTALL_DIR_RELEASE" `
    -DBUILD_SHARED_LIBS=FALSE `
    -DZENOHCXX_ZENOHC=ON `
    -DZENOHCXX_ZENOHPICO=OFF `
    -DCMAKE_BUILD_TYPE=Release `
    -DCMAKE_PREFIX_PATH="$INSTALL_DIR_RELEASE"

# 构建所有目标
cmake --build . --config Release
cmake --install . --config Release

# 构建并运行测试
cmake --build . --target tests --config Release
ctest -C Release -V

cmake --build . --target examples --config Release

Write-Host "Build completed!"
```

#ubuntu 安装脚本
```
#!/bin/bash

# 版本信息
ZENOHCPP_VERSION="1.1.0"

# 设置基础路径
BASE_DIR="$HOME/git/zenoh-cpp"
ZENOHC_DIR="$BASE_DIR/zenoh-c"
INSTALL_DIR_DEBUG="$HOME/local/debug"
INSTALL_DIR_RELEASE="$HOME/local/release"

# 检查 Rust 是否已安装
if ! command -v cargo &> /dev/null; then
    echo "Error: Rust (cargo) is not installed"
    echo "Please install Rust first using:"
    echo "curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh"
    exit 1
fi

# 确保安装目录存在
mkdir -p "$INSTALL_DIR_DEBUG"
mkdir -p "$INSTALL_DIR_RELEASE"

# 加载 Rust 环境（如果需要）
if [ -f "$HOME/.cargo/env" ]; then
    source "$HOME/.cargo/env"
fi

# 更新代码和切换到指定版本
cd "$BASE_DIR"
git fetch --all --tags
git checkout $ZENOHCPP_VERSION
git submodule update --init --recursive

# 首先编译 zenoh-c (子模块)
echo "Building zenoh-c..."
cd "$ZENOHC_DIR"

# Debug 版本 zenoh-c
echo "Building zenoh-c Debug version..."
rm -rf build-debug
mkdir -p build-debug
cd build-debug

# 设置 Rust 环境变量
export RUSTFLAGS="-g"
export CARGO_HOME="$HOME/.cargo"
export RUST_BACKTRACE=1

cmake .. \
    -DCMAKE_INSTALL_PREFIX="$INSTALL_DIR_DEBUG" \
    -DBUILD_SHARED_LIBS=FALSE \
    -DBUILD_TESTING=ON \
    -DBUILD_EXAMPLES=ON \
    -DCMAKE_BUILD_TYPE=Debug

cmake --build . --config Debug -- -j$(nproc)
cmake --install .

# Release 版本 zenoh-c
echo "Building zenoh-c Release version..."
cd "$ZENOHC_DIR"
rm -rf build-release
mkdir -p build-release
cd build-release

# 设置 Rust 环境变量
export RUSTFLAGS="-C opt-level=3"
export CARGO_HOME="$HOME/.cargo"

cmake .. \
    -DCMAKE_INSTALL_PREFIX="$INSTALL_DIR_RELEASE" \
    -DBUILD_SHARED_LIBS=FALSE \
    -DBUILD_TESTING=ON \
    -DBUILD_EXAMPLES=ON \
    -DCMAKE_BUILD_TYPE=Release

cmake --build . --config Release -- -j$(nproc)
cmake --install .

# 然后编译 zenoh-cpp
echo "Building zenoh-cpp..."

# Debug 版本 zenoh-cpp
echo "Building zenoh-cpp Debug version..."
cd "$BASE_DIR"
rm -rf build-debug
mkdir -p build-debug
cd build-debug

cmake .. \
    -DCMAKE_INSTALL_PREFIX="$INSTALL_DIR_DEBUG" \
    -DBUILD_SHARED_LIBS=FALSE \
    -DZENOHCXX_ZENOHC=ON \
    -DZENOHCXX_ZENOHPICO=OFF \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_PREFIX_PATH="$INSTALL_DIR_DEBUG"

# 构建所有目标
cmake --build . --config Debug -- -j$(nproc)
cmake --install . --config Debug

# 构建并运行测试
cmake --build . --target tests --config Debug
ctest -C Debug -V
cmake --build . --target examples --config Debug

# Release 版本 zenoh-cpp
echo "Building zenoh-cpp Release version..."
cd "$BASE_DIR"
rm -rf build-release
mkdir -p build-release
cd build-release

cmake .. \
    -DCMAKE_INSTALL_PREFIX="$INSTALL_DIR_RELEASE" \
    -DBUILD_SHARED_LIBS=FALSE \
    -DZENOHCXX_ZENOHC=ON \
    -DZENOHCXX_ZENOHPICO=OFF \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_PREFIX_PATH="$INSTALL_DIR_RELEASE"

# 构建所有目标
cmake --build . --config Release -- -j$(nproc)
cmake --install . --config Release

# 构建并运行测试
cmake --build . --target tests --config Release
ctest -C Release -V

cmake --build . --target examples --config Release

echo "Build completed!"

```



### 这是对应的 PowerShell 脚本版本：

build.ps1:
```
# 设置基础路径
$BASE_DIR = "D:\git\zenoh-cpp"
$INSTALL_DIR_DEBUG = "D:\local\debug"
$INSTALL_DIR_RELEASE = "D:\local\release"

# 确保安装目录存在
New-Item -ItemType Directory -Force -Path $INSTALL_DIR_DEBUG
New-Item -ItemType Directory -Force -Path $INSTALL_DIR_RELEASE

# 更新代码和切换到1.1.0版本
Set-Location $BASE_DIR
git fetch --all --tags
git checkout 1.1.0
git submodule update --init --recursive

# Debug 版本构建
Write-Host "Building Debug version..." -ForegroundColor Green
if (Test-Path "build-debug") {
    Remove-Item -Recurse -Force build-debug
}
New-Item -ItemType Directory -Force -Path build-debug
Set-Location build-debug

cmake .. -G "Visual Studio 17 2022" -A x64 `
    -DCMAKE_INSTALL_PREFIX="$INSTALL_DIR_DEBUG" `
    -DBUILD_SHARED_LIBS=FALSE `
    -DZENOHCXX_ZENOHC=ON `
    -DZENOHCXX_ZENOHPICO=OFF `
    -DCMAKE_BUILD_TYPE=Debug `
    -DCMAKE_VS_PLATFORM_TOOLSET=v143

# 构建所有目标
cmake --build . --config Debug
cmake --install . --config Debug
# 构建并运行测试
cmake --build . --target tests --config Debug
ctest -C Debug -V

# Release 版本构建
Write-Host "Building Release version..." -ForegroundColor Green
Set-Location $BASE_DIR
if (Test-Path "build-release") {
    Remove-Item -Recurse -Force build-release
}
New-Item -ItemType Directory -Force -Path build-release
Set-Location build-release

cmake .. -G "Visual Studio 17 2022" -A x64 `
    -DCMAKE_INSTALL_PREFIX="$INSTALL_DIR_RELEASE" `
    -DBUILD_SHARED_LIBS=FALSE `
    -DZENOHCXX_ZENOHC=ON `
    -DZENOHCXX_ZENOHPICO=OFF `
    -DCMAKE_BUILD_TYPE=Release `
    -DCMAKE_VS_PLATFORM_TOOLSET=v143

# 构建所有目标
cmake --build . --config Release
cmake --install . --config Release
# 构建并运行测试
cmake --build . --target tests --config Release
ctest -C Release -V

Write-Host "Build completed!" -ForegroundColor Green
```

### 单独的 Debug 和 Release 脚本：

build-debug.ps1:
```
# 设置基础路径
$BASE_DIR = "D:\git\zenoh-cpp"
$INSTALL_DIR_DEBUG = "D:\local\debug"

# 确保安装目录存在
New-Item -ItemType Directory -Force -Path $INSTALL_DIR_DEBUG

# 更新代码和切换到1.1.0版本
Set-Location $BASE_DIR
git fetch --all --tags
git checkout 1.1.0
git submodule update --init --recursive

# Debug 版本构建
Write-Host "Building Debug version..." -ForegroundColor Green
if (Test-Path "build-debug") {
    Remove-Item -Recurse -Force build-debug
}
New-Item -ItemType Directory -Force -Path build-debug
Set-Location build-debug

cmake .. -G "Visual Studio 17 2022" -A x64 `
    -DCMAKE_INSTALL_PREFIX="$INSTALL_DIR_DEBUG" `
    -DBUILD_SHARED_LIBS=FALSE `
    -DZENOHCXX_ZENOHC=ON `
    -DZENOHCXX_ZENOHPICO=OFF `
    -DCMAKE_BUILD_TYPE=Debug `
    -DCMAKE_VS_PLATFORM_TOOLSET=v143

# 构建所有目标
cmake --build . --config Debug
cmake --install . --config Debug
# 构建并运行测试
cmake --build . --target tests --config Debug
ctest -C Debug -V

Write-Host "Debug build completed!" -ForegroundColor Green
```

build-release.ps1:
```

# 设置基础路径
$BASE_DIR = "D:\git\zenoh-cpp"
$INSTALL_DIR_RELEASE = "D:\local\release"

# 确保安装目录存在
New-Item -ItemType Directory -Force -Path $INSTALL_DIR_RELEASE

# 更新代码和切换到1.1.0版本
Set-Location $BASE_DIR
git fetch --all --tags
git checkout 1.1.0
git submodule update --init --recursive

# Release 版本构建
Write-Host "Building Release version..." -ForegroundColor Green
if (Test-Path "build-release") {
    Remove-Item -Recurse -Force build-release
}
New-Item -ItemType Directory -Force -Path build-release
Set-Location build-release

cmake .. -G "Visual Studio 17 2022" -A x64 `
    -DCMAKE_INSTALL_PREFIX="$INSTALL_DIR_RELEASE" `
    -DBUILD_SHARED_LIBS=FALSE `
    -DZENOHCXX_ZENOHC=ON `
    -DZENOHCXX_ZENOHPICO=OFF `
    -DCMAKE_BUILD_TYPE=Release `
    -DCMAKE_VS_PLATFORM_TOOLSET=v143

# 构建所有目标
cmake --build . --config Release
cmake --install . --config Release
# 构建并运行测试
cmake --build . --target tests --config Release
ctest -C Release -V

Write-Host "Release build completed!" -ForegroundColor Green
```

使用方法：

1. 以管理员身份运行 PowerShell
2. 设置执行策略（如果需要）：
```
Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser
```

3. 运行脚本：
```
# 构建所有版本
.\build.ps1

# 或者单独构建Debug版本
.\build-debug.ps1

# 或者单独构建Release版本
.\build-release.ps1
```

注意事项：
1. 脚本使用 Visual Studio 2022 构建系统（可根据需要修改）
2. 路径使用了 D: 盘，请根据实际情况修改路径
3. 默认使用 v143 工具集，可以根据需要修改
4. 如果遇到执行策略问题，需要调整 PowerShell 执行策略
5. 构建前会自动清理之前的构建目录
6. 使用彩色输出以提高可读性

先决条件：
1. 安装 Visual Studio 2022 并选择 C++ 开发工具
2. 安装 Git
3. 安装 CMake


#linux环境安装库到系统目录

# 需要 root 权限
sudo cp -r /home/tao/local/release/include/ /usr/local/include/
sudo cp -r /home/tao/local/release/lib/ /usr/local/lib/
sudo cp -r /home/tao/local/release/lib/cmake/ /usr/local/lib/cmake/


# 需要 root 权限
sudo cp -r /root/local/release/include/ /usr/local/include/
sudo cp -r /root/local/release/lib/ /usr/local/lib/
sudo cp -r /root/local/release/lib/cmake/ /usr/local/lib/cmake/

# 更新动态库缓存
sudo ldconfig