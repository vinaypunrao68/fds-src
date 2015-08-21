#!/bin/bash
WORKSPACE=$1
config=starwars

# deploy inventory file/confs
echo "Deploy inventory file - config: $config"
cp $WORKSPACE/source/test/perf/dell_test/$config $WORKSPACE/ansible/inventory/

# bring up fds
echo "Bring up FDS"
pushd $WORKSPACE/ansible
./scripts/deploy_fds.sh $config local
echo "Sleep for one minute more..."
sleep 120
popd
echo "Ready."

