#!/bin/bash

# 设置版本和路径变量
DATABUS_VERSION="v0.0.2"
HARBOR_URL="harbor.bigdata.seisys.cn:8443"
IMAGE_NAME="v2x/sadip_x64"
DATABUS_PATH="/root/sadip"

source /etc/profile

mkdir -p /var/lib/rancher/k3s /etc/rancher/k3s

# 检查是否为 Jetson 平台
if [[ $(uname -r) == *"tegra"* ]]; then
    echo "Jetson (Orin) platform detected. Enabling Jetson GPU support."
    GPU_OPTIONS="--runtime=nvidia"
    GPU_ENV="--device=/dev/nvidia0 \
             --device=/dev/nvhost-ctrl \
             --device=/dev/nvhost-ctrl-gpu \
             --device=/dev/nvhost-prof-gpu \
             --device=/dev/nvmap \
             --device=/dev/nvhost-gpu \
             --device=/dev/nvhost-as-gpu \
             --device=/dev/nvidia-uvm \
             --device=/dev/nvidia-uvm-tools \
             -v /usr/lib/aarch64-linux-gnu:/usr/lib/aarch64-linux-gnu \
             -v /usr/local/cuda:/usr/local/cuda \
             -e NVIDIA_VISIBLE_DEVICES=all \
             -e NVIDIA_DRIVER_CAPABILITIES=compute,utility"
    IMAGE_NAME="v2x/sadip_cuda_armv8"

# 检查是否有 NVIDIA 独立显卡
elif command -v nvidia-smi &> /dev/null && nvidia-smi &> /dev/null; then
    echo "NVIDIA discrete GPU detected. Enabling GPU support."
    GPU_OPTIONS="--runtime=nvidia"
    GPU_ENV="--gpus all \
             -e NVIDIA_VISIBLE_DEVICES=all \
             -e NVIDIA_DRIVER_CAPABILITIES=compute,utility"
    IMAGE_NAME="v2x/sadip_cuda_x64"

else
    echo "No GPU detected or not functioning. Running without GPU support."
    GPU_OPTIONS=""
    GPU_ENV=""
fi

# 完整的镜像地址
FULL_IMAGE_NAME="${HARBOR_URL}/${IMAGE_NAME}:${DATABUS_VERSION}"

# 拉取新镜像
echo "Pulling new image: ${FULL_IMAGE_NAME}"
#podman pull ${FULL_IMAGE_NAME}

# 运行新容器（使用 --replace 自动替换同名容器）
echo "Starting new container with version ${DATABUS_VERSION}"
if podman run -d --restart=always --privileged --net=host --pid=host --userns=host --cgroupns=host --name sadip --replace \
 ${GPU_OPTIONS} \
 ${GPU_ENV} \
 -v ${DATABUS_PATH}/apps:/root/sadip/apps \
 -v ${DATABUS_PATH}/config:/root/sadip/config \
 -v ${DATABUS_PATH}/s3watch:/root/sadip/s3watch \
 -v ${DATABUS_PATH}/logs:/root/sadip/logs \
 -v ${DATABUS_PATH}/images:/root/sadip/images \
 -v ${DATABUS_PATH}/containers:/var/lib/containers \
 -v /run/systemd/journal/socket:/run/systemd/journal/socket \
 -v /sys/fs/cgroup:/sys/fs/cgroup \
 -v /run/systemd/system:/run/systemd/system \
 -v /run/dbus/system_bus_socket:/run/dbus/system_bus_socket \
 -v /var/lib/rancher/k3s:/var/lib/rancher/k3s \
 -v /etc/rancher/k3s:/etc/rancher/k3s \
 ${FULL_IMAGE_NAME}
then
    echo "Container started successfully"
else
    echo "Failed to start container. Please check the error messages above."
    exit 1
fi
