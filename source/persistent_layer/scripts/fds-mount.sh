#!/bin/bash

fdsroot=/fds
xfspart=3

mkdir -p ${fdsroot}
hddroot=${fdsroot}/hdd
ssdroot=${fdsroot}/ssd

for d in {a..k}; do
	dev=/dev/sd$(echo "$d" | tr "0-9a-z" "1-9a-z_")
	mntdir=${hddroot}/sd${d}
	echo "Mount XFS ${mntdir} to dev ${dev}"
	mkdir -p ${mntdir}
	mount ${dev}${xfspart} ${mntdir}
done

xfspart=1
mntdir=${ssdroot}/sda
echo "Mount XFS ${mntdir} to dev /dev/sdm"
mkdir -p ${mntdir}
mount /dev/sdm1 ${mntdir}

mntdir=${ssdroot}/sdb
echo "Mount XFS ${mntdir} to dev /dev/sdn"
mkdir -p ${mntdir}
mount /dev/sdn1 ${mntdir}
