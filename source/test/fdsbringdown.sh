#!/bin/bash
kill -9 `pgrep AMAgent`
kill -9 `pgrep StorMgr`
kill -9 `pgrep DataMgr`
kill -9 `pgrep orchMgr`
cd Build/linux*/bin
rm -rf *ldb
rm -rf logs stats
rm -f *.log
rm -rf SNode*
rm -rf ObjRankDB
rm -f ocache_stats.dat

rm -rf /fds/hdd
rm -rf /fds/ssd
rm -rf /fds/objStats
