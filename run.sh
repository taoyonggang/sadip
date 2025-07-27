#!/bin/bash

#/lib/systemd/systemd &
#dockerd-entrypoint.sh &
iptables -P FORWARD ACCEPT

chown -R root:root /run

nohup ./sync_time_loop.sh > /dev/null 2>&1 &

/usr/sbin/sshd -D &

#sleep 5

./config/run-app.sh
