#!/bin/sh

# Run from the top level source dir
#
cd Build/linux-x86_64.debug/bin
rm -rf *.ldb
rm -rf logs stats
rm -rf node?_*
rm -rf SNodeVolIndex*
rm -rf SNodeObjIndex*
rm om.*.log
rm am.*.log
rm sm.*.log
rm -rf /fds/hdd/sd?
rm -rf /fds/ssd/sd?
