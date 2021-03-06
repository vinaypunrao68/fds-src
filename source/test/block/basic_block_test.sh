#!/bin/sh
device=$1


sudo dd if=random1 of=$device bs=4096 count=1 oflag=direct,dsync conv=fdatasync status=none
sudo dd if=$device of=random1.out bs=4096 count=1 iflag=direct,dsync conv=fdatasync status=none
echo test block aligned read/write
sudo cmp --bytes=4096 random1 random1.out
if [ $? -ne 0 ];then
    echo "compare failed!"
fi

sudo dd if=random2 of=$device bs=4096 count=16 oflag=direct,dsync conv=fdatasync status=none
sudo dd if=$device of=random2.out bs=4096 count=16 iflag=direct,dsync conv=fdatasync status=none
echo test overwrite
sudo cmp --bytes=65536 random2 random2.out
if [ $? -ne 0 ];then
    echo "compare failed!"
fi

sudo dd if=$device of=random2.out bs=1024 count=64 iflag=direct,dsync conv=fdatasync status=none
echo test sub-block aligned read
sudo cmp --bytes=65536 random2 random2.out
if [ $? -ne 0 ];then
    echo "compare failed!"
fi

sudo dd if=$device of=random2.out bs=16K count=4 iflag=direct,dsync conv=fdatasync status=none
echo test read larger than a block
sudo cmp --bytes=65536 random2 random2.out
if [ $? -ne 0 ];then
    echo "compare failed!"
fi

sudo dd if=random3 of=$device bs=1024 count=64 oflag=direct,dsync conv=fdatasync status=none
sudo dd if=$device of=random3.out bs=4096 count=16 iflag=direct,dsync conv=fdatasync status=none
echo test sub-block aligned write
sudo cmp --bytes=65536 random3 random3.out
if [ $? -ne 0 ];then
    echo "compare failed!"
fi

echo test sub-block aligned r/w
sudo dd if=$device of=random3.out bs=1024 count=64 iflag=direct,dsync conv=fdatasync status=none
sudo cmp --bytes=65536 random3 random3.out
if [ $? -ne 0 ];then
    echo "compare failed!"
fi

sudo dd if=random1 of=$device bs=16K count=4 oflag=direct,dsync conv=fdatasync status=none
sudo dd if=$device of=random1.out bs=4096 count=16 iflag=direct,dsync conv=fdatasync status=none
echo test write larger than a block
sudo cmp --bytes=65536 random1 random1.out
if [ $? -ne 0 ];then
    echo "compare failed!"
fi
