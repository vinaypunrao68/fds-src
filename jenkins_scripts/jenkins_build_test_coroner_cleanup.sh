#!/bin/bash -l

. jenkins_scripts/jenkins_system_test.lib


# For long system test, disable only DmMigrationFailover
DISABLED_SYSTEM_TEST_SCENARIO_LIST="DmMigrationFailover"

if [[ "${1}" == "smoke_test_only" ]]
then
    SYSTEM_TEST_SCENARIO_LIST="BuildSmokeTest_onpr"
elif [[ "${1}" == "jenkins_build_on_master_commit" ]]
then
    # For master mergess, disable some tests
    DISABLED_SYSTEM_TEST_SCENARIO_LIST="ActiveIORestartTest RestartDataPersistence ActiveIOKillTest ActiveIORndKillTest MultiAMVolOpsTest RestartClusterKillServices StaticMigration BuildSmokeTest_onpr QosTest DmMigrationFailover"
fi

for test_case in ${DISABLED_SYSTEM_TEST_SCENARIO_LIST}
do
    SYSTEM_TEST_SCENARIO_LIST=${SYSTEM_TEST_SCENARIO_LIST/${test_case}/}
done

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
