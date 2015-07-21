#~/bin/bash

tail=`date +%s`

echo ${tail}



dd if=/dev/sda1 of=sda1.${tail} status=none

for disk in /dev/sd[b-z]
do
    echo "Processing ${disk}"
    dd if=${disk} of=${disk##*/}.${tail} bs=1M count=1
done
