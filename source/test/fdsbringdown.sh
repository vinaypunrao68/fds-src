#!/bin/bash
pkill -9 AMAgent
pkill -9 Mgr
pkill -9 platformd
pkill -9 -f com.formationds.web.om.Main

if [ -e test/cleanup.sh ]; then
    test/cleanup.sh
elif [ -e cleanup.sh ]; then
    cleanup.sh
fi
