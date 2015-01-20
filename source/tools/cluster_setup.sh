#!/bin/bash

machines=`echo $1 | sed -e 's/,/ /g'`
main_node_ip=$2

echo "Machines: $machines"
echo "Main node ip: $main_node_ip"

pushd ../
tar czvf /tmp/fds-package.tgz Build/linux-x86_64.release/bin Build/linux-x86_64.release/lib Build/linux-x86_64.release/tools
tar czvf /tmp/fds-tools.tgz tools test/fds-tool.py
tar czvf /tmp/fds-config.tgz config/etc
tar czvf /tmp/fdslib.tgz test/fdslib
#cp /home/monchier/regress/fds-src/source/test/fdslib/*.py /usr/local/lib/python2.7/dist-packages/fdslib

for m in $machines ; do
    scp /tmp/fds-package.tgz /tmp/fds-tools.tgz /tmp/fds-config.tgz /tmp/fdslib.tgz $m:/tmp/
    scp tools/remote_setup.sh $m:/tmp/
    ssh $m "cd /tmp/ && bash remote_setup.sh $main_node_ip"
    done
popd

