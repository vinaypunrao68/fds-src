#!/bin/bash 
./disk.py /tmp/iostat.rd iops reads
./disk.py /tmp/iostat.rd kbreads reads
./disk.py /tmp/iostat.rd kbwrites reads
./disk.py /tmp/iostat.wr iops writes
./disk.py /tmp/iostat.wr kbreads writes
./disk.py /tmp/iostat.wr kbwrites writes
pushd /tmp
tar czvf tests.tgz *.png
popd
