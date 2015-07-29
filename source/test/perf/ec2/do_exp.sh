#!/bin/bash

workspace=/home/pairing/projects/fds-src
cwd=$PWD

boot_fds=0
stop_fds=0

while getopts bs opt; do
  case $opt in
  b)
      boot_fds=1
      ;;
  s)
      stop_fds=1
      ;;
  esac
done

shift $((OPTIND - 1))

echo "boot: $boot_fds"
echo "stop: $stop_fds"

if [ $stop_fds -eq 1 ]; then
    pushd $workspace/ansible
    ./scripts/teardown_ec2_cluster.sh matteo_ec2
    popd
fi

if [ $boot_fds -eq 1 ]; then
    pushd $workspace/ansible
    ./scripts/deploy_fds_ec2.sh matteo_ec2 8 nightly | tee $cwd/log
    popd
fi

om_host=`cat log |sed -n -e '/start_om/,$p'| head -2|tail -1| sed -e  "s/.*\[//" | sed -e  "s/\]//"`
echo "om_host: $om_host"
sed -e "s/OM_HOST/$om_host/" fdscli.conf > ~/.fdscli.conf
cat log |sed -n -e '/PLAY RECAP/,$p'|grep 'failed=0'|grep -v localhost |grep -v awo|awk '{print $1}' > hosts
cat hosts
echo "Installing software"
for h in `cat hosts` ; do ./install_sw.sh $h; done
echo "Running tests"
./run_ec2_tests.sh $workspace hosts
