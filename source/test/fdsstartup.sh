#!/bin/bash
cd Build/linux*/bin
./orchMgr > ./orchMgr.out &
sleep 2

./DataMgr > ./DataMgr.out  &
sleep 2

./StorMgr > ./StorMgr.out &
sleep 2

./AMAgent > ./AMAgent.out &
sleep 2
