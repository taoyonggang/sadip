#!/bin/bash

iptables -P FORWARD ACCEPT &

nohup podman system service --time=0  2>&1 &

sleep 5

nohup podman run -d -p 9001:9001 --restart=always --name portainer_agent --replace \
  -v /var/run/podman/podman.sock:/var/run/docker.sock \
  -v /var/lib/containers/storage/volumes/:/var/lib/docker/volumes \
  harbor.bigdata.seisys.cn:8443/v2x/portainer/agent:2.22.0-linux-amd64-alpine  2>&1 &

nohup k3s server '--cluster-cidr=172.20.0.0/16' '--service-cidr=172.21.0.0/16' '--flannel-backend=host-gw' '--snapshotter=fuse-overlayfs' '--kube-apiserver-arg=kubelet-preferred-address-types=InternalIP,ExternalIP,Hostname'  2>&1 &

./node -f config/node.ini
