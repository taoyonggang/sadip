#!/bin/bash

# 确保目标目录存在
mkdir -p /usr/local/lib

# 获取 Conan 缓存目录
CONAN_HOME="${HOME}/.conan2"

# 定义要复制的库文件模式
PATTERNS=(
    "libaws-*"
    "libboost_*"
    "libcrypto*"
    "libssl*"
    "libz.*"
    "libPoco*"
    "libprotobuf*"
    "libsoci*"
    "libsqlite*"
    "libmysql*"
    "libcurl*"
)

echo "Copying libraries from Conan cache..."

# 复制每个模式匹配的库文件
for pattern in "${PATTERNS[@]}"; do
    echo "Copying $pattern"
    find "${CONAN_HOME}/p" -type f -name "${pattern}.so*" -exec cp -v {} /usr/local/lib/ \;
done

# 设置合适的权限
chmod 644 /usr/local/lib/*

echo "Library copy completed. Checking results:"
ls -l /usr/local/lib/