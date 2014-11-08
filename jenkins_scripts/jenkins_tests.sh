#!/bin/bash -l

ulimit -c unlimited
ulimit -n 400000

#Report the current ulimits
echo "Current ulimit settings:"
ulimit -a

function unit_tests
{
   echo "RUNNING UNIT TEST"

   # Run Unit Test
   cd ${root_dir}/jenkins_scripts
   ./run-unit-tests.py
   [[ $? -ne 0 ]] && echo "UNIT TEST:  FAILED" && exit 98
   cd -
}

function system_tests
{
   echo "RUNNING SYSTEM TEST"

   # Run Unit Test
   cd ${root_dir}/source/test/testsuites
   ./BuildSmokeTest.py -q BuildSmokeTest_oncommit.ini -d dummy
   [[ $? -ne 0 ]] && echo "SYSTEM TEST:  FAILED" && exit 97
   cd -
}

function fds_start
{
   echo "STARTING FDS"

   # bring-up FDS in simulation mode
   source/tools/fds stop
   source/tools/fds cleanstart
}

function fds_smoketest
{
   echo "RUNNING SMOKE TEST"

   # Running smoke test
   AM_IP=$(grep `hostname` /etc/hosts | awk '{print $1}')
   source/test/fds-primitive-smoke.py --up false --down false --am_ip $AM_IP
   [[ $? -ne 0 ]] && echo "SMOKE TEST:  FAILED" && exit 96
}

function fds_stop
{
   echo "STOPPING FDS"

   # Tear down
   source/tools/fds stop
}

root_dir=$(pwd)

unit_tests
system_tests

exit 0
