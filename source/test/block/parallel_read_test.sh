#!/bin/sh
device=$1

mkdir -p out

echo Writing data to $device
sudo dd if=random1 of=$device bs=4096 count=16 oflag=direct,dsync conv=fdatasync status=none

echo Testing parallel_read_load
cat parallel_read_load | sudo xargs -n 1 -P 8 dd if=$device bs=4096 iflag=direct count=16 status=none

