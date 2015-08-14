#!/bin/bash

workspace=/home/pairing/projects/fds-src
cwd=$PWD

boot_fds=0
stop_fds=0
num_nodes=

while getopts ":bsn:" opt; do
  case $opt in
  b)
      boot_fds=1
      ;;
  s)
      stop_fds=1
      ;;
  n)
      num_nodes=$OPTARG
      ;;
  esac
done

shift $((OPTIND - 1))

echo "boot: $boot_fds"
echo "stop: $stop_fds"
echo "num_nodes: $num_nodes"

if [ $stop_fds -eq 1 ]; then
    pushd $workspace/ansible
    ./scripts/teardown_ec2_cluster.sh matteo_ec2
    popd
fi

if [ $boot_fds -eq 1 ]; then
    pushd $workspace/ansible
    ./scripts/deploy_fds_ec2.sh matteo_ec2 $num_nodes nightly | tee $cwd/log
    popd
    om_host=`cat log |sed -n -e '/start_om/,$p'| head -2|tail -1| sed -e  "s/.*\[//" | sed -e  "s/\]//"`
    echo "om_host: $om_host"
    sed -e "s/OM_HOST/$om_host/" fdscli.conf > ~/.fdscli.conf
    cat log |sed -n -e '/PLAY RECAP/,$p'|grep 'failed=0'|grep -v localhost |grep -v awo|awk '{print $1}' > hosts
    cat hosts
    nodes_up=`cat hosts|wc -l`
    if [ "$nodes_up" -ne "$num_nodes" ]; then
        echo "Nodes up: $nodes_up < $num_nodes"
        exit 1
    fi
    echo "Installing software"
    for h in `cat hosts` ; do ./install_sw.sh $h; done
fi

echo "Running tests"
./run_ec2_tests.sh $workspace hosts
