#!/bin/bash

# 设置版本和路径变量
DATABUS_VERSION="v1.3.5"
HARBOR_URL="harbor.bigdata.seisys.cn:8443"
IMAGE_NAME="v2x/databus2_armv8"
DATABUS_PATH="/home/dgtw/databus2"

source /etc/profile

mkdir -p /var/lib/rancher/k3s /etc/rancher/k3s



# 检查 NVIDIA GPU 是否可用
if command -v nvidia-smi &> /dev/null && nvidia-smi &> /dev/null
then
    echo "NVIDIA GPU detected. Enabling GPU support."
#    GPU_OPTIONS="--runtime=nvidia"
    GPU_ENV="--gpus ALL"
	IMAGE_NAME="v2x/databus2_cuda_armv8"
else
    echo "NVIDIA GPU not detected or not functioning. Running without GPU support."
    GPU_OPTIONS=""
    GPU_ENV=""
fi

# 完整的镜像地址
FULL_IMAGE_NAME="${HARBOR_URL}/${IMAGE_NAME}:${DATABUS_VERSION}"
# 拉取新镜像
echo "Pulling new image: ${FULL_IMAGE_NAME}"
docker pull ${FULL_IMAGE_NAME}

# 停止并删除旧容器（如果存在）
if docker ps -a --format '{{.Names}}' | grep -q "^incm-databus$"; then
    echo "Stopping and removing old container"
    docker stop incm-databus
    docker rm incm-databus
else
    echo "No existing container named incm-databus found"
fi

# 运行新容器
echo "Starting new container with version ${DATABUS_VERSION}"
if docker run -d --restart=always --privileged --net=host --pid=host --userns=host --cgroupns=host --name incm-databus \
 ${GPU_OPTIONS} \
 ${GPU_ENV} \
 -v ${DATABUS_PATH}/apps:/root/databus2/apps \
 -v ${DATABUS_PATH}/config:/root/databus2/config \
 -v ${DATABUS_PATH}/s3watch:/root/databus2/s3watch \
 -v ${DATABUS_PATH}/logs:/root/databus2/logs \
 -v ${DATABUS_PATH}/images:/root/databus2/images \
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
