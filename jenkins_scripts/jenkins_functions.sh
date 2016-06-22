#!/bin/bash -lxe

DESIRED_SHM_SIZE="3G"      # In gigs, in full Gigs only.
KILL_LIST=(bare_am AmFunctionalTest NbdFunctionalTest)

export CCACHE_DIR="/dev/shm/.ccache"

# Unit test directories for Python
PYTHON_COVERAGE_TEST_RUNNER_DIRECTORIES="source/platform/python/tests"
PYTHON_UNITTEST_DISCOVERY_DIRECTORIES="source/tools"

# This list of tests to be run
# The list of system test scenarios, do not include the .ini"
# This variable defines if tests are running in simulated mode or hardware nodes.
HARDWARE_ENVIRONMENT=false
INVENTORY=''

# Runsuites.cfg runs 1.Snapshottest 2. RestartDataPersistance 3.Expunge 4.Amfailover on same set of volumes to reduce jenkins runtime.
SYSTEM_TEST_SCENARIO_LIST="ActiveIORestartTest ActiveIORndKillTest MultiAMVolOpsTest BuildSmokeTest_onpr"


# For long system test, disable only DmMigrationFailover
# TODO pooja once fs-6439 is fixed then re enable ExpungeTest
DISABLED_SYSTEM_TEST_SCENARIO_LIST="QosTest DmMigrationFailover ActiveMigration"

if [[ "${1}" == "smoke_test_only" ]]
then
    SYSTEM_TEST_SCENARIO_LIST="BuildSmokeTest_onpr"
elif [[ "${1}" == "jenkins_build_on_master_commit" ]]
then
    # ActiveMigration is disabled for master merge because scenarios are not supported with VG enabled
    # As of 03/03/2016 only ActiveIORestartTest runs for master merges
    # For master mergess, disable most tests
    DISABLED_SYSTEM_TEST_SCENARIO_LIST="Runsuites ActiveIORndKillTest MultiAMVolOpsTest BuildSmokeTest_onpr"
fi

for test_case in ${DISABLED_SYSTEM_TEST_SCENARIO_LIST}
do
    SYSTEM_TEST_SCENARIO_LIST=${SYSTEM_TEST_SCENARIO_LIST/${test_case}/}
done


function performance_report
{
    unit=${1}
    seconds=${2}

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

# Wrapper to pushd to control output
function do_pushd
{
    pushd ${1} > /dev/null
}

# Wrapper to popd to control output
function do_popd
{
    popd > /dev/null
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
    ps axww > source/cit/ps-out-`date +%Y%m%d%M%S`.${1}.txt
}

function startup
{
    message "LOGGING env settings"
    source_jdk
    env
    capture_process_list ${FUNCNAME}
}

function configure_symlinks
{
    message "IIIII RUNNING /fds symlink configuration"
    do_pushd source
    ./dev_make_install.sh
    do_popd
}

function configure_console_access
{
    do_pushd source/tools
    ./fdsconsole.py accesslevel debug
    do_popd
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
    [[ ! -d /fds/var/log/corefiles ]] && mkdir -p /fds/var/log/corefiles
    sysctl -w "kernel.core_pattern=/fds/var/log/corefiles/%e-%p-%u-%t.core"
    ulimit -n 400000

    message "CURRENT ulimit settings"

    ulimit -a
}

function source_jdk
{
    message "source ansible/files/jdk.sh to set correct JAVA_HOME"

    . ansible/files/jdk.sh
}

function jenkins_workspace_cleanup
{
    local workspace_dir="/home/jenkins/workspace"
    loop_count=0

    if [[ ${#JENKINS_URL} -lt 1 || ! -d ${workspace_dir} ]]
    then
       return
    fi

    message "Cleaning up oldest jenkins workspace builds to ensure resources exist to complete a successful build."

    #Check the amount of free space on the builder.  Remove old jobs from the workspace until 35GB is free."
    while [ ${loop_count} -lt 10 -a $( df -B G ${workspace_dir} | tail -1 | tr -d G | awk '{print $4}' ) -lt 35 ]
    do
        dirs=$( ls -rt1 ${workspace_dir} )

        for d in ${dirs}
        do
            if [[ ${JOB_NAME} == ${d} ]]
            then
                continue
            fi

            if [[ $( du -sm ${workspace_dir}/${d} | cut -f1 ) -gt 1 ]]
            then
                echo "Cleaning ${workspace_dir}/${d}"
                rm -rf ${workspace_dir}/${d}
                break;
            fi
        done

        error_trap_disabled
        let loop_count++
        error_trap_enabled
    done

    if [[ ${loop_count} -gt 9 ]]
    then
        echo " ERROR Can not recover enough disk space to ensure a clean build"
        exit 67
    fi
}

function clean_up_environment
{
   message "IIIII Clean up pre build environment "
   message "KILLING any extraneous running processes"
   for proc in ${KILL_LIST[@]}
   do
       echo "Checking ${proc}"
       pkill -9 -f ${proc} || true
   done

   message "STOPPING redis"
   sudo source/tools/redis.sh stop

   jenkins_workspace_cleanup

   message "Forcing removal of fds-deps"
   sudo dpkg -P fds-deps

   # Clean remove the old /fds
   message "CLEAN existing /fds"
   rm -rf /fds || true

   #  This could be improved by looking in more places and for specific core files (e.g., from FDS processes and test tools)
   message "CLEAN existing core files (prebuild)"

   for dir in /corefiles /fds/var/log/corefiles
   do
       if [[ ! -e ${dir} ]]
       then
           continue
       fi

       for mask in '*.core' '*.hprof' '*hs_err_pid*.log'
       do
           echo find ${dir} -type f -name "${mask}" -delete
       done
   done

   message " Remove old source/cit logs"
   rm -rf ${TEST_WORKSPACE}/source/cit/*
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

function check_for_cores
{
    core_hunter
    return_code=$?

    if [[ ${return_code} -eq 0 ]]
    then
        echo ""
        echo "OHHHHH NO, an ERROR has occurred return_code = ${return_code}"
        echo ""
        echo "Build aborted due to the presence of core files on the builder following a system test scenario:"
        ls -la /fds/var/log/corefiles
        run_coroner 1
    fi
}

function python_coverage_test_runner_tests
{
    for directory in ${PYTHON_COVERAGE_TEST_RUNNER_DIRECTORIES}
    do
        do_pushd ${directory}
        python -m CoverageTestRunner
        if [[ $? -ne 0 ]]
        then
            message "EEEEE Python unit test problem(s) detected running in ${directory}"
            run_coroner 1
        fi
        do_popd
    done
}

function python_unittest_discovery
{
    for directory in ${PYTHON_UNITTEST_DISCOVERY_DIRECTORIES}
    do
        do_pushd ${directory}
        python -m unittest discover -p "*_test.py"
        if [[ $? -ne 0 ]]
        then
            message "EEEEE Python unit test problem(s) detected running in ${directory}"
            run_coroner 1
        fi
        do_popd
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

function check_cpp_cit_results
{
    do_pushd ${WORKSPACE}
    error_count=$( find source/cit -name "*.xml" |xargs grep "errors=\"" |grep -v "errors=\"0" |wc -l )
    failure_count=$( find source/cit -name "*.xml" |xargs grep "failures=\"" |grep -v "failures=\"0" |wc -l  )

    if [[ ${error_count} -gt 0 || ${failure_count} -gt 0 ]]
    then
        message "EEEEE Unit test errors (${error_count}) and/or failures (${failure_count}) detected"
        find source/cit -name "*.xml" |xargs grep "errors=\"" |grep -v "errors=\"0"
        find source/cit -name "*.xml" |xargs grep "failures=\"" |grep -v "failures=\"0"
        run_coroner 1
    fi

    do_popd
}

function run_cpp_unit_tests
{
    message "IIIII RUNNING c++ unit tests"

    # Run Unit Test
    do_pushd jenkins_scripts
    start_time=$(date +%s)
    ./run-unit-tests.py

    if [[ $? -ne 0 ]]
    then
        message "EEEEE Unit test problem(s) detected"
        run_coroner 1
    fi

    check_cpp_cit_results

    end_time=$(date +%s)
    performance_report UNIT_TESTS $(( ${end_time} - ${start_time} ))
    do_popd
}

function check_xunit_failures
{
    message "Checking xunit output for failure, system test:  ${1}"
    grep -e 'failures="[1-9].*"' `find source/cit/ -name '*.xml'`
    [[ $? -eq 0 ]]
}

function check_xunit_errors
{
    message "Checking xunit output for errors, system test:  ${1}"
    grep -e 'errors="[1-9].*"' `find source/cit/ -name '*.xml'`
    [[ $? -eq 0 ]]
}

function deployment_error
{
    message "Problem(s) detected in packaging fds-platform/deb or deployment"
    run_coroner 1
}

function system_test_error
{
    scenario=${1}
    message "EEEEE System Test problem(s) detected running ${scenario}"
    do_pushd ${TEST_WORKSPACE}
    check_xunit_errors ${scenario}
    check_xunit_failures ${scenario}
    do_popd
    run_coroner 1
}

function system_test_scenario_wrapper
{
    scenario=${1}

    do_pushd source/test/testsuites

    message "Report disk free prior to test:  ${scenario}"
    echo "   df output"
    df -B G /home/jenkins/workspace
    
    echo "   du output"
    du -sm /home/jenkins/workspace/* |sort -rn

    message "IIIII RUNNING System Test Scenario:  ${scenario}"
    sudo ./ScenarioDriverSuite.py -q ./${scenario}.ini --verbose || system_test_error ${scenario}
    echo "***** Scenario complete:  ${scenario} passed"

    do_popd

    capture_process_list SysTest.${scenario}
    check_xunit_errors ${scenario}
    check_xunit_failures ${scenario}
    check_for_cores
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
    exitCode=${1}

    do_pushd ${WORKSPACE}

    message "IIIII RUNNING post build node cleanup"

    if [[ ${#JENKINS_URL} -gt 0 ]]
    then
        message "Cleaning jenkins build slave."
        jenkins_scripts/python/cleanup_jenkins_slave.py
        cleanupExitCode=$?
    else
        echo "No cleanup performed, this script is not being executed on a jenkins build slave."
    fi

    capture_process_list ${FUNCNAME}

    do_popd

    if [[ $exitCode -eq 0 ]]
    then
        if [[ ${cleanupExitCode} -ne 0 ]]
        then
            exit ${cleanupExitCode}
        fi
    fi
    exit ${exitCode}
}

function run_coroner
{
    excludes_dest="/fds/sbin"

    [[ ! -e ${excludes_dest} ]] && mkdir -p ${excludes_dest}

    start_time=$(date +%s)

    if [[ ${#JENKINS_URL} -gt 0 ]]
    then
        REFID=${BUILD_TAG}
        TEST_WORKSPACE=${WORKSPACE}
    else
        REFID="local_build_at_`date +%s`"
        TEST_WORKSPACE=${local_build_root}
    fi

    if [ ${HARDWARE_ENVIRONMENT} = true ] ; then
        message "IIIII RUNNING coroner playbook"
        ansible-playbook -i ../../../ansible/inventory/${INVENTORY} ../../../ansible/playbooks/coroner.yml -e coroner_fdsroot=/fds -e coroner_push=yes -e coroner_dumpdir=coroner_playbook
    else
        DUMP_LOCATION=bld-dump
        # This test is primarily designed to work KVM build slaves.
        if [[ "$( ip -o -4 addr show dev eth0 )" =~ "10.1.16" ]]
        then
            DUMP_LOCATION=fre-dump
        fi

        message "IIIII RUNNING coroner"

        do_pushd ${TEST_WORKSPACE}

        cp ansible/files/coroner.excludes ${excludes_dest}/.
        source/tools/coroner.py collect --refid $REFID --buildermode \
          --collect-dirs build_debug_bin:source/Build/linux-x86_64.debug/bin        \
                         build_release_bin:source/Build/linux-x86_64.release/bin    \
                         test_logs:source/cit                                       \
                         fds-node1:/fds/node1                                       \
                         fds-node2:/fds/node2                                       \
                         fds-node3:/fds/node3                                       \
                         fds-node4:/fds/node4                                       \

        for file in /tmp/fdscoroner*.tar.gz
        do
            sshpass -p share rsync -aHv -e "ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null -q" --progress ${file} share@${DUMP_LOCATION}:jenkins-run_coroner/
            rm ${file}
        done

        do_popd

        rm -rf /tmp/fdscoroner*
    fi
    rm -f fds/var/log/corefiles/*
    rm -f /fds/var/cores/*
    rm -rf ${TEST_WORKSPACE}/source/cit/*

    end_time=$(date +%s)

    performance_report RUN_CORONER $(( ${end_time} - ${start_time} ))

    run_node_cleanup ${1}
}

function run_test_scenarios_on_hardware_nodes
{
    HARDWARE_ENVIRONMENT=true
    INVENTORY=$1; shift
    HARDWARE_LST_TEST_SCENARIO_LIST=( "$@" )

    message "Packaging fds-platform"
    sudo make package fds-platform BUILD_TYPE=debug || deployment_error
    chown -R $(whoami) /home/$(whoami)/*
    chown -R $(whoami) /home/$(whoami)/.*
    echo 'Formation1234' > /home/$(whoami)/.vault_pass.txt
    echo 'Formation1234' > /root/.vault_pass.txt

    message "IIIII Deploying local build on 4 bare metal nodes with inventory ${INVENTORY}"
    ansible/scripts/deploy_fds.sh ${INVENTORY} local || deployment_error

    start_time=$(date +%s)

    for scenario in ${HARDWARE_LST_TEST_SCENARIO_LIST[@]}; do
        hardware_test_scenario_wrapper ${INVENTORY} ${scenario}
    done

    end_time=$(date +%s)

    performance_report SYSTEM_TESTS $(( ${end_time} - ${start_time} ))
}

function hardware_test_scenario_wrapper
{
    INVENTORY=$1
    scenario=$2

    do_pushd source/test/testsuites
    # TODO pooja: fix  fs-4375
    message " IIIII On hardware nodes running System Test: ${scenario}"
    sudo ./ScenarioDriverSuite.py -q ./${scenario}.ini -d dummy --verbose -l ../../cit/ --install -z ${INVENTORY} --reusecluster || system_test_error ${scenario}

    echo " *** Scenario completed :  ${scenario} passed"
    do_popd

    capture_process_list SysTest.${scenario}
    check_for_cores
}
