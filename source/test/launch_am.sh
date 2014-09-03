#/bin/bash
om_ip=10.1.10.139
echo "Launching platformd with om_ip=$om_ip"
pushd /fds/bin
./platformd --fds-root=/fds --fds.plat.id=node1 --fds.plat.platform_port=7000 --fds.plat.om_ip=$om_ip > pm.log 2>&1 &
sleep 10
echo "Launching AM with om_ip=$om_ip"
export LD_LIBRARY_PATH=/fds/lib:/usr/local/lib:/usr/lib/jvm/java-8-oracle/jre/lib/amd64; export PATH=$PATH:/fds/bin; cd /fds/bin; ulimit -c unlimited; ulimit -n 12800; ./AMAgent --fds-root=/fds > am.log 2>&1 &
sleep 20
echo "Done."
popd
