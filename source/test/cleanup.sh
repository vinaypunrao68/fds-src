#!/bin/sh

# Run from the top level source dir
#
cd Build/linux*/bin
rm -rf *.ldb
rm -rf logs stats
rm -rf node?_*
rm om_?.log
rm sh_?.log
rm sm_?.log
rm sm_*.log
rm dm_?.log
rm -rf /fds/hdd/sd?
rm -rf /fds/ssd/sd?
