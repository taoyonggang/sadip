#!/bin/bash

podman build --build-arg https_proxy=$HTTP_PROXY --build-arg http_proxy=$HTTP_PROXY  --build-arg HTTP_PROXY=$HTTP_PROXY --build-arg HTTPS_PROXY=$HTTP_PROXY . -t harbor.bigdata.seisys.cn:8443/v2x/databus2_x64:v1.0.8 -f databus2/docker/Dockerfile.v2.x64

podman build --build-arg https_proxy=$HTTP_PROXY --build-arg http_proxy=$HTTP_PROXY  --build-arg HTTP_PROXY=$HTTP_PROXY --build-arg HTTPS_PROXY=$HTTP_PROXY . -t harbor.bigdata.seisys.cn:8443/v2x/databus2_cuda_x64:v1.0.8 -f databus2/docker/Dockerfile.cuda.v2.x64

podman build --build-arg https_proxy=$HTTP_PROXY --build-arg http_proxy=$HTTP_PROXY  --build-arg HTTP_PROXY=$HTTP_PROXY --build-arg HTTPS_PROXY=$HTTP_PROXY . -t harbor.bigdata.seisys.cn:8443/v2x/databus2_humble_cuda_x64:v1.0.8 -f databus2/docker/Dockerfile.humble.cuda.v2.x64

 podman build --build-arg https_proxy=$HTTP_PROXY --build-arg http_proxy=$HTTP_PROXY  --build-arg HTTP_PROXY=$HTTP_PROXY --build-arg HTTPS_PROXY=$HTTP_PROXY . -t harbor.bigdata.seisys.cn:8443/v2x/databus2_armv8:v1.0.8 -f databus2/docker/Dockerfile.v2.armv8
 
 
 podman build --build-arg https_proxy=$HTTP_PROXY --build-arg http_proxy=$HTTP_PROXY  --build-arg HTTP_PROXY=$HTTP_PROXY --build-arg HTTPS_PROXY=$HTTP_PROXY . -t harbor.bigdata.seisys.cn:8443/v2x/sadip_x64:v1.0.8 -f sadip/docker/Dockerfile.v3.x64

podman build --build-arg https_proxy=$HTTP_PROXY --build-arg http_proxy=$HTTP_PROXY  --build-arg HTTP_PROXY=$HTTP_PROXY --build-arg HTTPS_PROXY=$HTTP_PROXY . -t harbor.bigdata.seisys.cn:8443/v2x/sadip_cuda_x64:v1.0.8 -f sadip/docker/Dockerfile.cuda.v3.x64

podman build --build-arg https_proxy=$HTTP_PROXY --build-arg http_proxy=$HTTP_PROXY  --build-arg HTTP_PROXY=$HTTP_PROXY --build-arg HTTPS_PROXY=$HTTP_PROXY . -t harbor.bigdata.seisys.cn:8443/v2x/sadip_humble_cuda_x64:v1.0.8 -f sadip/docker/Dockerfile.humble.cuda.v3.x64

 podman build --build-arg https_proxy=$HTTP_PROXY --build-arg http_proxy=$HTTP_PROXY  --build-arg HTTP_PROXY=$HTTP_PROXY --build-arg HTTPS_PROXY=$HTTP_PROXY . -t harbor.bigdata.seisys.cn:8443/v2x/sadip_armv8:v1.0.8 -f sadip/docker/Dockerfile.v3.armv8