#!/bin/bash -le

ulimit -c unlimited
ulimit -n 400000

#Report the current ulimits

ulimit -a

function unit_tests
{
   echo "RUNNING UNIT TEST"

   # Run Unit Test
   cd ${root_dir}/jenkins_scripts
   ./run-unit-tests.py
   cd -
}


function system_tests
{
   echo "RUNNING SYSTEM TEST"
   
   # Run Unit Test
   cd ${root_dir}/source/test/testsuites
   ./BuildSmokeTest.py -q BuildSmokeTest_oncommit.ini -d dummy
   cd -
}


function fds_start
{
   echo "STARTING FDS"

   # bring-up FDS in simulation mode
   set +e
   source/tools/fds stop
   source/tools/fds cleanstart
   set -e
}


function fds_smoketest
{
   echo "RUNNING SMOKE TEST"
   
   # Running smoke test
   AM_IP=$(grep `hostname` /etc/hosts | awk '{print $1}')
   source/test/fds-primitive-smoke.py --up false --down false --am_ip $AM_IP
}


function fds_stop
{
   echo "STOPPING FDS"
   
   # Tear down
   set +e
   source/tools/fds stop
   true
   set -e
}

root_dir=$(pwd)

unit_tests
system_tests
fds_start
sleep 5
fds_smoketest
fds_stop
