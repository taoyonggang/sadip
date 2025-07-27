#!/bin/bash

# 需要 root 权限运行
if [ "$(id -u)" -ne 0 ]; then
   echo "此脚本需要 root 权限，请使用 sudo 运行" 
   exit 1
fi

echo "===== 开始配置文件句柄限制为 500000 ====="

# 1. 修改 /etc/security/limits.conf
echo "正在配置 /etc/security/limits.conf..."
cat << EOF > /etc/security/limits.conf
# /etc/security/limits.conf
#
# 每个用户的限制配置文件
#
#<domain>      <type>  <item>         <value>
#

# 为所有用户设置文件句柄限制
*               soft    nofile          500000
*               hard    nofile          500000
root            soft    nofile          500000
root            hard    nofile          500000

# End of file
EOF
echo "✓ /etc/security/limits.conf 已配置"

# 2. 清除任何可能覆盖的限制文件
echo "正在检查并清理 /etc/security/limits.d/ 目录..."
if [ -d "/etc/security/limits.d" ]; then
    mkdir -p /etc/security/limits.d.backup
    mv /etc/security/limits.d/* /etc/security/limits.d.backup/ 2>/dev/null
    echo "创建新的文件句柄限制配置..."
    cat << EOF > /etc/security/limits.d/20-nofile.conf
# 为所有用户设置统一的文件句柄限制
*               soft    nofile          500000
*               hard    nofile          500000
root            soft    nofile          500000
root            hard    nofile          500000
EOF
    echo "✓ 已清理并创建新的 limits.d 配置"
else
    echo "✓ limits.d 目录不存在，跳过"
fi

# 3. 配置 PAM 以加载 limits 模块
echo "正在配置 PAM..."
for PAM_FILE in /etc/pam.d/common-session /etc/pam.d/common-session-noninteractive /etc/pam.d/login; do
    if [ -f "$PAM_FILE" ]; then
        if ! grep -q "pam_limits.so" "$PAM_FILE"; then
            echo "session required pam_limits.so" >> "$PAM_FILE"
            echo "✓ 已将 pam_limits.so 添加到 $PAM_FILE"
        else
            echo "✓ $PAM_FILE 已包含 pam_limits.so"
        fi
    fi
done

# 4. 配置 systemd 默认限制
echo "正在配置 systemd 默认限制..."
for SYSTEMD_CONF in /etc/systemd/system.conf /etc/systemd/user.conf; do
    if [ -f "$SYSTEMD_CONF" ]; then
        sed -i '/DefaultLimitNOFILE=/d' "$SYSTEMD_CONF"
        echo "DefaultLimitNOFILE=500000" >> "$SYSTEMD_CONF"
        echo "✓ 已配置 $SYSTEMD_CONF"
    fi
done

# 5. 配置系统级文件最大数
echo "正在配置系统级文件句柄限制..."
if ! grep -q "fs.file-max" /etc/sysctl.conf; then
    echo "fs.file-max = 50000" >> /etc/sysctl.conf
    echo "✓ 已添加 fs.file-max 到 /etc/sysctl.conf"
else
    sed -i 's/fs.file-max[ ]*=[ ]*[0-9]*/fs.file-max = 50000/' /etc/sysctl.conf
    echo "✓ 已更新 fs.file-max 在 /etc/sysctl.conf"
fi

# 6. 应用 sysctl 更改
echo "正在应用 sysctl 更改..."
sysctl -p
echo "✓ sysctl 更改已应用"

# 7. 确保 SSH 配置正确
if [ -f "/etc/ssh/sshd_config" ]; then
    echo "正在检查 SSH 配置..."
    if ! grep -q "^UsePAM yes" /etc/ssh/sshd_config; then
        sed -i 's/^#\?UsePAM .*/UsePAM yes/' /etc/ssh/sshd_config
        systemctl restart sshd
        echo "✓ SSH 配置已更新并重启服务"
    else
        echo "✓ SSH 已正确配置"
    fi
fi

# 8. 添加到全局 profile
echo "正在更新全局 profile..."
if [ -f "/etc/profile" ]; then
    if ! grep -q "ulimit -n" /etc/profile; then
        echo -e "\n# 增加默认文件句柄限制\nulimit -n 500000" >> /etc/profile
        echo "✓ 已更新 /etc/profile"
    else
        sed -i 's/ulimit -n [0-9]*/ulimit -n 500000/' /etc/profile
        echo "✓ 已更新 /etc/profile 中的文件句柄设置"
    fi
fi

# 9. 创建一个简单的验证脚本
echo "正在创建验证脚本..."
cat << 'EOF' > /usr/local/bin/check-limits.sh
#!/bin/bash
echo "当前文件句柄限制: $(ulimit -n)"
echo "系统最大文件数: $(cat /proc/sys/fs/file-max)"
echo "当前进程限制:"
cat /proc/self/limits | grep "open files"
EOF
chmod +x /usr/local/bin/check-limits.sh
echo "✓ 验证脚本已创建，运行 'check-limits.sh' 检查限制"

echo "===== 配置完成 ====="
echo "需要重启系统才能使所有更改生效。"
echo "重启后，运行 'check-limits.sh' 验证更改是否生效。"
echo "是否立即重启系统? (y/n)"
read -r response
if [[ "$response" =~ ^([yY][eE][sS]|[yY])$ ]]; then
    echo "系统将在 5 秒后重启..."
    sleep 5
    reboot
else
    echo "请记得稍后重启系统以应用所有更改。"
fi
