#!/bin/sh
set -e

if [ ! -f /etc/yeetmouse.conf ]; then
    exit 0
fi

for i in $(seq 1 50); do
    if [ -e /sys/module/yeetmouse/parameters/update ]; then
        break
    fi
    sleep 0.1
done

if [ ! -e /sys/module/yeetmouse/parameters/update ]; then
    echo "YeetMouse parameters did not appear in time"
    exit 1
fi

chown root:yeetmouse /sys/module/yeetmouse/parameters/*
exec /usr/bin/yeetmousectl apply /etc/yeetmouse.conf
