#!/bin/bash -l

echo "###################################################################################"
echo "###################################################################################"
echo "###################################################################################"
echo "###################################################################################"
echo "# This script is deprecated by jenkins_tests_copy_for_build_test_coroner_cleanup.sh"
echo "###################################################################################"
echo "###################################################################################"
echo "###################################################################################"
echo "###################################################################################"
echo ""
echo "$0 is deprecated, please update your workflow to use jenkins_build_test_coroner_cleanup.sh (pausing 30 seconds)"

curl "http://beacon.formationds.com/?JENKINS_DEPRECATED_SCRIPTS=true&script=${0}&JOB_NAME=${JOB_NAME}&PWD=${PWD}&BUILD_NUMBER=${BUILD_NUMBER}" &

sleep 30

function message
{
   echo "================================================================================"
   echo "$*"
   echo "================================================================================"
}

message "CONFIGURE ulimit AND COREFILE SETTINGS"

ulimit -c unlimited
mkdir -p /corefiles
sysctl -w "kernel.core_pattern=/corefiles/%e-%p-%u-%t.core"
ulimit -n 400000

message "CURRENT ulimit SETTINGS"

ulimit -a


function performance_report
{
   unit=$1
   seconds=$2

   if [[ ${seconds} -lt 60 ]]
   then
      message "${unit} TIME = ${seconds} seconds"
   else
      message "${unit} TIME = ${seconds} seconds or $(( ${seconds} / 60 )) minute(s) $(( ${seconds} % 60 )) seconds"
   fi
}

function python_unit_tests
{
   message "RUNNING PYTHON UNIT TESTS"

   # Run Unit Test
   cd ${root_dir}/jenkins_scripts
   start_time=$(date +%s)
   ./run-python-unit-tests.sh
   [[ $? -ne 0 ]] && echo "PYTHON UNIT TESTS:  FAILED" && exit 98
   end_time=$(date +%s)
   performance_report PYTHON_UNIT_TESTS $(( ${end_time} - ${start_time} ))
   cd -
}

function unit_tests
{
   message "RUNNING UNIT TESTS"

   # Run Unit Test
   cd ${root_dir}/jenkins_scripts
   start_time=$(date +%s)
   ./run-unit-tests.py
   [[ $? -ne 0 ]] && echo "UNIT TEST:  FAILED" && exit 98
   end_time=$(date +%s)
   performance_report UNIT_TEST $(( ${end_time} - ${start_time} ))
   cd -
}

function system_tests
{
   message "RUNNING SYSTEM TEST"

   # Run Unit Test
   cd ${root_dir}/source/test/testsuites
   start_time=$(date +%s)
   ./ScenarioDriverSuite.py -q ./BuildSmokeTest_onpr.ini -d dummy --verbose
   [[ $? -ne 0 ]] && echo "SYSTEM TEST:  FAILED" && exit 97
   end_time=$(date +%s)
   performance_report SYSTEM_TEST $(( ${end_time} - ${start_time} ))
   cd -
   
   # Run QoS test.
   cd "${root_dir}"/source/test/testsuites
   start_time="$(date +%s)"
   ./ScenarioDriverSuite.py -q ./QosTest.ini -d dummy --verbose
   [[ $? -ne 0 ]] && echo "SYSTEM TEST:  FAILED" && exit 97
   end_time=$(date +%s)
   performance_report QOS_TEST $(( ${end_time} - ${start_time} ))
   cd -
}

root_dir=$(pwd)

python_unit_tests
unit_tests
system_tests

exit 0
