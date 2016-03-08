#!/bin/bash -l
# This script is used for running system tests on AWS nodes for jenkins
# parameter1: first argument passed is always inventory
# parameter2: second argument onwards is a list of the tests to run. Length on the this list can be varied for every run
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

startup
clean_up_environment
configure_cache
configure_symlinks
configure_limits

error_trap_disabled

build_fds
cache_report

configure_console_access      # Must be complted after the build
run_python_unit_tests
run_cpp_unit_tests
run_aws_test_scenarios $@

run_node_cleanup 0            # Completed successfully, cleanup and exit with a 0
