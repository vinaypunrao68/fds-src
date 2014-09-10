#!/bin/bash
pushd /home/monchier/FDS/source/Build/linux-x86_64.debug/bin
volumes="volume0 volume1 volume2 volume3"
#for v in $volumes
#do
#    ./fdscli --volume-modify $volume -s 1000  -g <iops_min> -m <iopsmax>  -r 10
#done
# assuming max is arong 2700
# everything limited to 2000
# ./fdscli --volume-modify volume0 -s 1000000  -g 1000 -m 2000 -r 10
# ./fdscli --volume-modify volume1 -s 1000000  -g 1000 -m 2000 -r 10
# ./fdscli --volume-modify volume2 -s 1000000  -g 1000 -m 2000 -r 10
# ./fdscli --volume-modify volume3 -s 1000000  -g 1000 -m 2000 -r 10
# sleep 5
# want to give minimum of 3100 to volume1
./fdscli --volume-modify volume1 -s 1000000  -g 3200 -m 5000 -r 10
# sleep 5
# volume2 minimum of 3000
# ./fdscli --volume-modify volume2 -s 1000000  -g 3000 -m 5000 -r 10
# sleep 5
# everybody back to normal
# ./fdscli --volume-modify volume0 -s 1000000  -g 2000 -m 4000 -r 10
# ./fdscli --volume-modify volume1 -s 1000000  -g 2000 -m 4000 -r 10
# ./fdscli --volume-modify volume2 -s 1000000  -g 2000 -m 4000 -r 10
# ./fdscli --volume-modify volume3 -s 1000000  -g 2000 -m 4000 -r 10
popd
