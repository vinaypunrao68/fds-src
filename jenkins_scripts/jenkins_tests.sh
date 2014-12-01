#!/bin/bash -l

echo "================================================================================"
echo "CONFIGURE ulimit AND COREFILE SETTINGS"
echo "================================================================================"

ulimit -c unlimited
mkdir -p /corefiles
sysctl -w "kernel.core_pattern=/corefiles/%e-%p.core"
ulimit -n 400000

echo "================================================================================"
echo "CURRENT ulimit SETTINGS"
echo "================================================================================"

ulimit -a

function unit_tests
{
   echo "================================================================================"
   echo "RUNNING UNIT TEST"
   echo "================================================================================"

   # Run Unit Test
   cd ${root_dir}/jenkins_scripts
   ./run-unit-tests.py
   [[ $? -ne 0 ]] && echo "UNIT TEST:  FAILED" && exit 98
   cd -
}

function system_tests
{
   echo "================================================================================"
   echo "RUNNING SYSTEM TEST"
   echo "================================================================================"

   # Run Unit Test
   cd ${root_dir}/source/test/testsuites
   ./BuildSmokeTest.py -q BuildSmokeTest_oncommit.ini -d dummy
   [[ $? -ne 0 ]] && echo "SYSTEM TEST:  FAILED" && exit 97
   cd -
}

root_dir=$(pwd)

unit_tests
system_tests

exit 0
