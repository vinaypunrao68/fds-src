#!/bin/bash

blk_shft=13
agcount=1024
sect_shft=13

for sd in /dev/sd[b-l]; do
	echo "Formating XFS to ${sd}3"
	mkfs.xfs ${sd}3 -f -q &
	#mkfs.xfs ${sd}3 -f -b log=$blk_shft -d agcount=$agcount -s log=$sect_shft -q &
done

wait
