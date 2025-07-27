#!/bin/bash

ln -s /home/dgtw /root/databus2/apps/dgtw
ln -s /home/yzc /root/databus2/apps/yzc
ln -s /etc/hosts /root/databus2/hosts
ln -s /etc/hostname /root/databus2/hostname

iptables -P FORWARD ACCEPT &

nohup podman system service --time=0  2>&1 &

sleep 15

CONTAINER_NAME="portainer_agent"

# 检查容器是否存在
if podman container exists $CONTAINER_NAME; then
    # 容器存在，检查是否正在运行
    if podman container inspect -f '{{.State.Running}}' $CONTAINER_NAME | grep -q "true"; then
        echo "容器 $CONTAINER_NAME 已经在运行中。"
    else
        echo "容器 $CONTAINER_NAME 存在但未运行，正在启动..."
        podman start $CONTAINER_NAME
    fi
else
    # 容器不存在，创建并启动
    echo "容器 $CONTAINER_NAME 不存在，正在创建并启动..."
    podman run -d -p 9001:9001 --restart=always --name $CONTAINER_NAME --replace \
      -v /var/run/podman/podman.sock:/var/run/docker.sock \
      -v /var/lib/containers/storage/volumes/:/var/lib/docker/volumes \
      harbor.bigdata.seisys.cn:8443/v2x/portainer/agent:2.22.0-linux-arm64-alpine
fi

nohup k3s server '--flannel-backend=host-gw' '--snapshotter=fuse-overlayfs' \
'--kube-apiserver-arg=kubelet-preferred-address-types=InternalIP,ExternalIP,Hostname'  2>&1 &

#nohup k3s server '--cluster-cidr=172.20.0.0/16' '--service-cidr=172.21.0.0/16' '--flannel-backend=host-gw' '--snapshotter=fuse-overlayfs' \
'--kube-apiserver-arg=kubelet-preferred-address-types=InternalIP,ExternalIP,Hostname'  2>&1 &

./node -f config/node.ini
