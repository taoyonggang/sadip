#!/bin/bash

# 检查 curl 是否安装，如果没有安装则安装 curl
if ! command -v curl &> /dev/null
then
    echo "curl could not be found, installing..."
    apt-get update
    apt-get install curl -y
fi

# 以非交互模式安装 Rust
echo "Installing Rust in non-interactive mode..."
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y --default-toolchain stable --profile default

# 设置环境变量
source $HOME/.cargo/env

# 配置中国内地源来加速后续的 cargo 包下载
echo "Configuring Chinese mainland mirrors for crates.io..."
mkdir -p ~/.cargo
cat <<EOF > ~/.cargo/config.toml
[source.crates-io]
replace-with = 'ustc'

[source.ustc]
registry = "https://mirrors.ustc.edu.cn/crates.io-index"
EOF

echo "Rust installation and configuration complete."
echo "Please restart your terminal or run 'source $HOME/.cargo/env' to use Rust."

source $HOME/.cargo/env
