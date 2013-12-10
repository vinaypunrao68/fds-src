export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/home/subbu/source/fds-src/source/../leveldb-1.12.0/:/home/subbu/source/fds-src/source/../Ice-3.5.0/cpp/lib/:/usr/local/lib; export ICE_HOME=/home/subbu/source/fds-src/source//home/subbu/source/fds-src/source/../Ice-3.5.0; cd /home/subbu/source/fds-src/source/Build/linux-*/bin;

./fdscli --volume-modify volume2 -s 100  -g 2 -m 50 -r 8 --om_ip=10.1.10.16 --om_port=8901
./fdscli --volume-modify volume3 -s 100  -g 2 -m 50 -r 8 --om_ip=10.1.10.16 --om_port=8901
./fdscli --volume-modify volume4 -s 100  -g 2 -m 50 -r 8 --om_ip=10.1.10.16 --om_port=8901

