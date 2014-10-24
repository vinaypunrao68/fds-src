#!/bin/bash -le

ulimit -c unlimited
ulimit -n 400000

function unit_tests
{
   echo "RUNNING UNIT TEST"
   find ${root_dir} -type d > ${root_dir}/files_before_unit_test
   # Run Unit Test
   cd ${root_dir}/jenkins_scripts
   ./run-unit-tests.py
   find ${root_dir} -type d > ${root_dir}/files_after_unit_test
   cd -
}


function system_tests
{
   echo "RUNNING SYSTEM TEST"
   find ${root_dir} -type d > ${root_dir}/files_before_system_test
   
   echo "Checking for xmlrunner"
   apt-cache search xml-runner
   
   # Run Unit Test
   cd ${root_dir}/source/test/testsuites
   
   ulimit -a
   
   ./BuildSmokeTest.py -q BuildSmokeTest_oncommit.ini -d dummy

   find ${root_dir} -type d > ${root_dir}/files_after_system_test
   cd -
}


function fds_start
{
   find ${root_dir} -type d > ${root_dir}/files_before_starting_fds

   # bring-up FDS in simulation mode
   set +e
   source/tools/fds stop
   source/tools/fds cleanstart
   set -e

   find ${root_dir} -type d > ${root_dir}/files_after_starting_fds
}


function fds_smoketest
{
   find ${root_dir} -type d > ${root_dir}/files_before_smoke_test
   # Running smoke test

   AM_IP=$(grep `hostname` /etc/hosts | awk '{print $1}')
   source/test/fds-primitive-smoke.py --up false --down false --am_ip $AM_IP
   find ${root_dir} > ${root_dir}/files_after_smoke_test
}


function fds_stop
{
   find ${root_dir} > ${root_dir}/files_before_stopping_fds
   # Tear down
   set +e
   source/tools/fds stop
   true
   set -e
   find ${root_dir} -type d > ${root_dir}/files_after_stopping_fds
}

root_dir=$(pwd)

unit_tests
system_tests
fds_start
sleep 5
fds_smoketest
fds_stop
