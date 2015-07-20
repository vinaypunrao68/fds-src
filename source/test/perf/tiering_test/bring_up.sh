WORKSPACE=$1
frequency=$2
batchsz=1024
config=starwars

machines="luke han c3po chewie"

#pushd $WORKSPACE/source/test/perf
#./is_fds_up.py -K --fds-nodes luke,han,c3po,chewie
#
#for m in luke han c3po chewie ; do
#    ssh $m dpkg -P fds-platform-rel
#done

# deploy inventory file/confs
echo "Deploy inventory file - config: $config"
inventory=$WORKSPACE/source/test/perf/tiering_test/$config
sed -e "s/BATCHSZ/$batchsz/g" $inventory | sed -e "s/FREQUENCY/$frequency/g" > .tmp
cp .tmp $WORKSPACE/ansible/inventory/$config


# bring up fds
echo "Bring up FDS"
pushd $WORKSPACE/ansible
./scripts/deploy_fds.sh $config nightly
if [ $? -ne 0 ]; then
    echo "FDS deploy has failed"
    exit 1
fi
echo "Sleep for one minute more..."
sleep 60
popd
echo "Ready."
