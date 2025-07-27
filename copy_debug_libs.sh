#!/bin/bash

# 确保目标目录存在
mkdir -p /usr/local/lib

# 获取 Conan 缓存目录
CONAN_HOME="${HOME}/.conan2"

echo "Copying libraries from Conan cache..."

# 查找并复制所有共享库文件
find "${CONAN_HOME}/p" -type f -name "*.so*" -exec cp -v {} /usr/local/lib/ \;

# 查找并复制所有静态库文件（如果需要的话）
find "${CONAN_HOME}/p" -type f -name "*.a" -exec cp -v {} /usr/local/lib/ \;

# 设置合适的权限
chmod 644 /usr/local/lib*

echo "Library copy completed. Checking results:"
ls -l /usr/local/lib