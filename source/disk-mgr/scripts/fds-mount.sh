#!/bin/sh

fdsroot=/fds/mnt
xfspart=3

mkdir -p ${fdsroot}
for dev in /dev/sd[b-l]; do
	echo "Mount XFS to dev $dev"
	mntdir=${fdsroot}/`basename ${dev}`
	mkdir -p ${mntdir}
	mount ${dev}${xfspart} ${mntdir}
done
