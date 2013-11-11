#!/bin/sh

for sd in /dev/sd[m-n]; do
	echo "Formating XFS to ${sd}1"
	mkfs.xfs ${sd}1 -f -q &
done
wait
