#!/bin/bash
cd Build/linux-x86_64.debug/bin

echo "Starting orchMgr"
./orchMgr > ./orchMgr.out &
sleep 2

echo "Starting DataMgr"
./DataMgr > ./DataMgr.out  &
sleep 2

echo "Starting StorMgr"
./StorMgr > ./StorMgr.out &
sleep 2

echo "Starting AMAgent"
./AMAgent > ./AMAgent.out &
sleep 2
