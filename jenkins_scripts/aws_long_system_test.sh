#!/bin/bash -l

. jenkins_scripts/jenkins_system_test.lib

error_trap_enabled
auto_locate

# Make sure to find includes.
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
    run_aws_test_scenarios
fi

run_node_cleanup 0            # Completed successfully, cleanup and exit with a 0
