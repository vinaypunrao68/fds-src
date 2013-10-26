#!/bin/bash

for sd in /dev/sd[c-l]; do
	echo "Partitioning $sd"
	parted -s $sd "mklabel gpt"
	parted -s -a none $sd "mkpart primary xfs 0 100MB"
	parted -s -a none $sd "mkpart primary xfs 100MB 32GB"
	parted -s -a none $sd "mkpart primary xfs 32GB -1"
done;
