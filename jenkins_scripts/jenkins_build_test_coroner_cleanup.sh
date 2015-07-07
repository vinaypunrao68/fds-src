#!/bin/bash -l

DESIRED_SHM_SIZE="3G"      # In gigs, in full Gigs only.
KILL_LIST=(bare_am AmFunctionalTest NbdFunctionalTest)

export CCACHE_DIR="/dev/shm/.ccache"


# Unit test directories for Python
PYTHON_COVERAGE_TEST_RUNNER_DIRECTORIES="source/platform/python/tests"
PYTHON_UNITTEST_DISCOVERY_DIRECTORIES="source/tools"

# The list of system test scenarios, do not include the .ini"
SYSTEM_TEST_SCENARIO_LIST="BuildSmokeTest_onpr ActiveMigration RestartDataPersistence ActiveIOKillTest ActiveIORndKillTest MultiAMVolOpsTest"
DISABLED_SYSTEM_TEST_SCENARIO_LIST="ActiveIORestartTest RestartClusterKillServices"   ## This should be deleted when fs-2473 fs-2478 are resolved

function message
{
    echo "================================================================================"
    echo "$*"
    echo "================================================================================"
}

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

function error_trap_enabled
{
    set -e
}

function error_trap_disabled
{
    set +e
}

function auto_locate
{
    # change to the fds-src directory.  This could be made more robust by digesting with $0, but should suffice for now.

    location="${PWD##*/}"
    jenkins_dir="jenkins_scripts"

    if [[ ${location} == ${jenkins_dir} ]]
    then
        cd ..
    fi

    if [[ ! -d ${jenkins_dir} ]]
    then
        echo "Please run this script from fds-src or ${jenkins_dir}"
        exit 1
    fi

    local_build_root="$PWD"
}

function capture_process_list
{
    funcname="$1"
    ps axww > source/cit/ps-out-`date +%Y%m%d%M%S`.${funcname}.txt
}

function startup
{
    message "LOGGING env settings"
    env
    capture_process_list ${FUNCNAME}
}

function configure_symlinks
{
    message "IIIII RUNNING /fds symlink configuration" 
    pushd source >/dev/null
    ./dev_make_install.sh
    popd > /dev/null
}

function configure_console_access
{
    pushd source/tools
    ./fdsconsole.py accesslevel debug
    popd
}

function configure_cache
{
    message "CONFIGURING /dev/shm size and ccache configuration"

    shm_size=$(df --human-readable --output=size /dev/shm/ | tail -1 | tr -d ' \.0')

    # Fix shm size if needed
    if [[ "${shm_size}" != "${DESIRED_SHM_SIZE}" ]]
    then
        sudo mount -t tmpfs -o remount,rw,nosuid,nodev,noexec,relatime,size=${DESIRED_SHM_SIZE} tmpfs /dev/shm
        echo "/dev/shm converted to ${DESIRED_SHM_SIZE}"
    fi

    # Configure the CCACHE size (lives in ${CCACHE_DIR} on Jenkins boxes)
    # This needs to be a bit smaller than ${DESIRED_SHM_SIZE}
    ccache -M 2.8G
    ccache -z
}

function configure_limits
{
    message "CONFIGURE ulimit and core file settings"

    ulimit -c unlimited
    [[ ! -d /corefiles ]] && mkdir -p /corefiles
    sysctl -w "kernel.core_pattern=/corefiles/%e-%p.core"
    ulimit -n 400000

    message "CURRENT ulimit settings"

    ulimit -a
}

function clean_up_environment
{
   message "KILLING any extraneous running processes"
   for proc in ${KILL_LIST[@]}
   do
       echo "Checking ${proc}"
       pkill -9 -f ${proc} || true
   done

   message "STOPPING redis"
   sudo source/tools/redis.sh stop

   # Clean remove the old /fds
   message "CLEAN exising /fds"
   rm -rf /fds || true

   message "CLEAN existing core files (prebuild)"
   #  This could be improved by looking in more places and for specific core files (e.g., from FDS processes and test tools)
   find /corefiles -type f -name "*.core" -print -delete

   capture_process_list ${FUNCNAME}
}

function build_fds
{
    error_trap_enabled

    start_time=$(date +%s)
    message "RUNNING devsetup"
    make devsetup
    end_time=$(date +%s)

    performance_report ANSIBLE $(( ${end_time} - ${start_time} ))
    start_time=$(date +%s)

    if [[ ${BUILD_TYPE} == 'release' ]] ; then
        message "BUILDING Formation Platform - BUILD_TYPE: release"
        jenkins_options="-r"
    else
        message "BUILDING Formation Platform - BUILD_TYPE: debug"
        if [[ ${COVERAGE} == 'true' ]]; then
           jenkins_options="-coverage"
        fi
    fi

    error_trap_disabled

    jenkins_scripts/build_fds.py ${jenkins_options}
    build_ret=$?

    end_time=$(date +%s)

    performance_report BUILD_FDS $(( ${end_time} - ${start_time} ))

    if [[ ${build_ret} -ne 0 ]]
    then
        message "EEEEE Build failure detected"
        run_coroner 1
    fi
}

function cache_report
{
   message "CCACHE post build statistics"
   ccache -s
}

function core_hunter
{
    message  "POKING around for core files"
    find /corefiles -type f -name "*.core" |grep -e ".*" > /dev/null
    return_code=$?

    if [[ ${return_code} -eq 0 ]]
    then
        echo ""
        echo "OHHHHH NO, an ERROR has occurred return_code = ${return_code}"
        echo ""
        echo "Build aborted due to the presence of core files on the builder following a system test scenario:"
        ls -la /corefiles
        run_coroner 1
    fi
}

function python_coverage_test_runner_tests
{
    for directory in ${PYTHON_COVERAGE_TEST_RUNNER_DIRECTORIES}
    do
        pushd ${directory}
        python -m CoverageTestRunner
        if [[ $? -ne 0 ]]
        then
            message "EEEEE Python unit test problem(s) detected running in ${directory}"
            run_coroner 1
        fi
        popd
    done
}

function python_unittest_discovery
{
    for directory in ${PYTHON_UNITTEST_DISCOVERY_DIRECTORIES}
    do
        pushd ${directory}
        python -m unittest discover -p "*_test.py"
        if [[ $? -ne 0 ]]
        then
            message "EEEEE Python unit test problem(s) detected running in ${directory}"
            run_coroner 1
        fi
        popd 
    done
}

function run_python_unit_tests
{
    message "IIIII RUNNING Python unit tests"

    # Run Unit Test
    start_time=$(date +%s)

    python_coverage_test_runner_tests
    python_unittest_discovery

    end_time=$(date +%s)
    performance_report PYTHON_UNIT_TESTS $(( ${end_time} - ${start_time} ))
}

function run_cpp_unit_tests
{
    message "IIIII RUNNING C++ unit tests"

    # Run Unit Test
    pushd jenkins_scripts
    start_time=$(date +%s)
    ./run-unit-tests.py

    if [[ $? -ne 0 ]]
    then
        message "EEEEE C++ unit test problem(s) detected"
        run_coroner 1
    fi

    end_time=$(date +%s)
    performance_report UNIT_TESTS $(( ${end_time} - ${start_time} ))
    popd
}

function check_xunit_failures
{
    message "Checking xunit output for failure"
    grep -e 'failures="[1-9].*"' `find source/cit/ -name '*.xml'`
    if [[ $? -eq 0 ]] ; then
        message "EEEEE Xunit Failures detected running System Test ${scenario}"
        run_coroner 1
    fi
}

function check_xunit_errors
{
    message "Checking xunit output for errors"
    grep -e 'errors="[1-9].*"' `find source/cit/ -name '*.xml'`
    if [[ $? -eq 0 ]] ; then
        message "EEEEE Xunit Errors detected running System Test ${scenario}"
        run_coroner 1
    fi
}

function system_test_scenario_wrapper
{
    scenario=${1}

    pushd source/test/testsuites

    message "IIIII RUNNING System Test Scenario:  ${scenario}"
    ./ScenarioDriverSuite.py -q ./${scenario}.ini -d dummy --verbose
    echo "***** Scenario complete:  ${scenario} complete - exit with: ${?}"

    popd

    capture_process_list ${FUNCNAME}.${scenario}

    check_xunit_errors ${scenario}
    check_xunit_failures ${scenario}

    if [[ $? -ne 0 ]]
    then
        message "EEEEE System Test problem(s) detected running ${scenario}"
        run_coroner 1
    fi

    core_hunter
}


function run_system_test_scenarios
{
    start_time=$(date +%s)

    for scenario in ${SYSTEM_TEST_SCENARIO_LIST} ${extra_system_tests}
    do
        system_test_scenario_wrapper ${scenario}
    done

    end_time=$(date +%s)

    performance_report SYSTEM_TESTS $(( ${end_time} - ${start_time} ))
}

function system_test_force_failure
{
    #keeping this around to assist testing in the future
    message "EEEEE System Test forced failure"
    run_coroner 1
}


function run_node_cleanup
{
    message "IIIII RUNNING post build node cleanup"

    if [[ ${#JENKINS_URL} -gt 0 ]]
    then
        echo "Cleaning jenkins build slave."
        jenkins_scripts/python/cleanup_jenkins_slave.py
    else
        echo "No cleanup performed, this script is not being executed on a jenkins build slave."
    fi

    capture_process_list ${FUNCNAME}

    exit $1
}


function run_coroner
{
    start_time=$(date +%s)

    if [[ ${#JENKINS_URL} -gt 0 ]]
    then
        REFID=${BUILD_TAG}
        DUMP_LOCATION=fre-dump
        TEST_WORKSPACE=${WORKSPACE}
    else
        REFID="local_build_at_`date +%s`"
        DUMP_LOCATION=fre-dump
        TEST_WORKSPACE=${local_build_root}
    fi

    message "IIIII RUNNING coroner"

    pushd ${TEST_WORKSPACE}

    source/tools/coroner.py collect --refid $REFID --collect-dirs build_debug_bin:source/Build/linux-x86_64.debug/bin build_release_bin:source/Build/linux-x86_64.release/bin fds-node1:/fds/node1 fds-node2:/fds/node2 fds-node3:/fds/node3 fds-node4:/fds/node4

    for file in /tmp/fdscoroner*.tar.gz
    do
        sshpass -p share scp ${file} share@${DUMP_LOCATION}:jenkins-run_coroner/
        rm ${file}
    done

    popd

    rm -rf /tmp/fdscoroner*
    rm -f /corefiles/*
    rm -f /fds/var/cores/*

    end_time=$(date +%s)

    performance_report RUN_CORONER $(( ${end_time} - ${start_time} ))

    run_node_cleanup $1
}

error_trap_enabled

auto_locate
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
run_system_test_scenarios

run_node_cleanup 0            # Completed successfully, cleanup and exit with a 0
