#!/bin/bash

# 配置
NTP_SERVER="10.10.1.103"  # 替换为您的 NTP 服务器
SYNC_INTERVAL=600  # 同步间隔，单位为秒（这里设置为10分钟）
LOG_FILE="/var/log/time_sync.log"  # 日志文件路径

# 检查是否已经有一个实例在运行
SCRIPT_NAME=$(basename "$0")
if pgrep -f "$SCRIPT_NAME" | grep -v ^$$ > /dev/null; then
    echo "Another instance of $SCRIPT_NAME is already running. Exiting."
    exit 1
fi

# 确保日志文件存在
touch "$LOG_FILE"

# 函数：执行时间同步
sync_time() {
    # 创建临时配置文件
    TEMP_CONF=$(mktemp)
    echo "server $NTP_SERVER iburst" > "$TEMP_CONF"

    # 执行时间同步
    ./chronyd -q -f "$TEMP_CONF" -U

    # 检查同步结果
    if [ $? -eq 0 ]; then
        echo "$(date): Time successfully synchronized with $NTP_SERVER" >> "$LOG_FILE"
    else
        echo "$(date): Error: Failed to synchronize time" >> "$LOG_FILE"
    fi

    # 删除临时配置文件
    rm -f "$TEMP_CONF"

    # 记录当前系统时间
    echo "$(date): Current system time: $(date)" >> "$LOG_FILE"
}

# 主循环
while true; do
    sync_time
    sleep $SYNC_INTERVAL
done
