#!/bin/bash
#
CurrentTimeZone=$(cat /etc/sysconfig/clock |cut -d'=' -f'2')
if [ ! $CurrentTimeZone == $1 ];then
cat << EOF > /etc/sysconfig/clock
ZONE=$1
EOF
ln -sf /usr/share/zoneinfo/$1 /etc/localtime
fi

