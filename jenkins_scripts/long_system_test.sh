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
. jenkins_scripts/jenkins_system_test.lib

# For long system test, do not disable any tests 
DISABLED_SYSTEM_TEST_SCENARIO_LIST=""

if [[ "${1}" == "smoke_test_only" ]]
then
    SYSTEM_TEST_SCENARIO_LIST="BuildSmokeTest_onpr"
fi

error_trap_enabled

auto_locate

# Now we are sure to find our "includes".
. ./jenkins_scripts/message.sh
. ./jenkins_scripts/core_hunter.sh

error_trap_disabled

# Check for special actions
if [[ "${1}" == "jenkins_build_aborted" ]]
then
    message "EEEEE Jenkins Build Aborted"
    run_coroner 1
fi

error_trap_enabled

startup
clean_up_environment
configure_cache
configure_symlinks
configure_limits

error_trap_disabled

build_fds
cache_report


if [[ "${1}" != "compile_only" ]]
then

    configure_console_access      # Must be complted after the build

    run_python_unit_tests
    run_cpp_unit_tests
    run_system_test_scenarios
fi

run_node_cleanup 0            # Completed successfully, cleanup and exit with a 0
