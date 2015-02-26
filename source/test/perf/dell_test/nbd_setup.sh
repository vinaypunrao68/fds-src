/fds/bin/fdscli --volume-create blockVolume -i 1 -s 10240 -p 50 -y blk
sleep 10
/fds/bin/fdscli --volume-modify blockVolume -s 10240 -g 0 -m 0 -r 10
sleep 10
../../../cinder/nbdadm.py attach c3po blockVolume

