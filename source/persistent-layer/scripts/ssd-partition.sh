#!/bin/sh

for sd in /dev/sd[m-n]; do
	echo "Partitioning $sd"
	parted -s $sd "mklabel gpt"
	parted -s -a none $sd "mkpart primary xfs 0 -1"
done;
