#!/bin/bash
mkdir -p  /var/lib/containers/storage/volumes
#/lib/systemd/systemd &
#dockerd-entrypoint.sh &
iptables -P FORWARD ACCEPT

chown -R root:root /run

nohup ./sync_time_loop.sh > /dev/null 2>&1 &

/usr/sbin/sshd -D &

sleep 2

bash ./config/run-app.sh 
