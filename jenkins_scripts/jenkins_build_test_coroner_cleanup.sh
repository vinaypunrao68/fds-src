#!/bin/bash -le

DESIRED_SHM_SIZE="3G"      # In gigs, in full Gigs only.
KILL_LIST=(bare_am AmFunctionalTest NbdFunctionalTest)

export CCACHE_DIR="/dev/shm/.ccache"

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

function startup
{
   message "LOGGING ENV SETTINGS"
   env
}

function configure_cache
{
   message "CONFIGURING SHM and CCACHE"

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

function clean_up_environment
{
   for proc in ${KILL_LIST[@]}
   do
       message "KILLING ANY RUNNING ${proc}"
       pkill -9 -f $proc || true
   done

   message "STOPPING redis"
   sudo source/tools/redis.sh stop

   # Clean remove the old /fds
   message "CLEANUP EXISTING /fds"
   rm -rf /fds || true

   message "CLEAN existing core files (prebuild)"
   #  This could be improved by looking in more places and for specific core files (e.g., from FDS processes and test tools)
   find /corefiles -type f -name "*.core" -print -delete
}

function build_fds
{
    start_time=$(date +%s)
    message "RUNNING DEVSETUP"
    make devsetup
    end_time=$(date +%s)

    performance_report ANSIBLE $(( ${end_time} - ${start_time} ))

    start_time=$(date +%s)
    if [[ ${BUILD_TYPE} == 'release' ]] ; then
        message "BUILDING FORMATION PLATFORM - BUILD_TYPE: release"
        jenkins_options="-r"
    else
        message "BUILDING FORMATION PLATFORM - BUILD_TYPE: debug"
        if [[ ${COVERAGE} == 'true' ]]; then
           jenkins_options="-coverage"
        fi
    fi

    set +e
    jenkins_scripts/build_fds.py ${jenkins_options}
    build_ret=$?
    set -e

message "build_ret = ${build_ret}"

    end_time=$(date +%s)

    performance_report BUILD_FDS $(( ${end_time} - ${start_time} ))

    [[ ${build_ret} -ne 0 ]] && return 1

    return 0
}

function cache_report
{
   message "CCACHE POSTBUILD STATISTICS"
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
        echo "OHHHHH NO, return_code = ${return_code}"
        echo ""
        echo "Build aborted due to the presence of core files on the builder following a system test scenario:"
        ls -la /corefiles
        /bin/false
    else
        /bin/true
    fi
}



















function from_jenkins
{

    ######### was #2

    ps axww > source/cit/ps-out-`date +%Y%m%d%M%S`.txt

    ###
    ###   This is an interim install step for running fds out of /fds/bin based on symlinks to the build tree.
    ###
    if [[ -e source/dev_make_install.sh ]]
    then
        echo "***** RUNNING /fds symlink configuration *****"
        cd source
        ./dev_make_install.sh
        cd ..
    fi
    ###
    ###   End of interim install step
    ###

    cd source/tools
    ./fdsconsole.py accesslevel debug
    cd ${WORKSPACE}

    jenkins_scripts/jenkins_tests.sh

    ps axww > source/cit/ps-out-`date +%Y%m%d%M%S`.txt


    jenkins_scripts/check_xunit_results.sh


    ######### was #3

    if [ ${ACTIVE_MIGRATION} == 'true' ] ; then

      cd source/test/testsuites

      echo "***** RUNNING ActiveMigration *****"
      ./ScenarioDriverSuite.py -q ./ActiveMigration.ini -d dummy --verbose
      echo "***** ActiveMigration complete - exit with: ${?} *****"

      cd ${WORKSPACE}

      ps axww > source/cit/ps-out-`date +%Y%m%d%M%S`.txt

      jenkins_scripts/check_xunit_results.sh
    fi









    ######### was #4

    # Run Restart Data Persistence Test

    cd source/test/testsuites
    echo "***** RUNNING RestartDataPersistence.ini *****"
    ./ScenarioDriverSuite.py -q ./RestartDataPersistence.ini -d dummy --verbose
    echo "***** RestartDataPersistence complete - exit with: ${?} *****"

    cd ${WORKSPACE}

    ps axww > source/cit/ps-out-`date +%Y%m%d%M%S`.txt

    jenkins_scripts/check_xunit_results.sh






    ######### was #5

    # Run Active IO Kill test

    cd source/test/testsuites

    echo "***** RUNNING ActiveIOKillTest.ini *****"
    ./ScenarioDriverSuite.py -q ./ActiveIOKillTest.ini -d dummy --verbose
    echo "***** ActiveIOKillTest complete - exit with: ${?} *****"

    cd ${WORKSPACE}

    ps axww > source/cit/ps-out-`date +%Y%m%d%M%S`.txt

    jenkins_scripts/check_xunit_results.sh








    ######### was #6

    # Run the Random IO Kill test

    cd source/test/testsuites

    echo "***** RUNNING ActiveIORndKillTest.ini *****"
    ./ScenarioDriverSuite.py -q ./ActiveIORndKillTest.ini -d dummy --verbose
    echo "***** ActiveIORndKillTest complete - exit with: ${?} *****"

    cd ${WORKSPACE}

    ps axww > source/cit/ps-out-`date +%Y%m%d%M%S`.txt

    jenkins_scripts/check_xunit_results.sh




    ######### was #7

    # Run the ActiveIORestartTest test

    if [ ${ACTIVEIO_RESTART} == 'true' ] ; then

      cd source/test/testsuites

      echo "***** RUNNING ActiveIORestartTest.ini *****"
      ./ScenarioDriverSuite.py -q ./ActiveIORestartTest.ini -d dummy --verbose
      echo "***** ActiveIORestartTest complete - exit with: ${?} *****"

      cd ${WORKSPACE}
      jenkins_scripts/check_xunit_results.sh
      echo "***** check_xunit_results exit with: ${?} *****"

    fi


    ######### was #8

    # Run the MultiAM test

    cd source/test/testsuites

    echo "***** RUNNING MultiAMVolOpsTest.ini *****"
    ./ScenarioDriverSuite.py -q ./MultiAMVolOpsTest.ini -d dummy --verbose
    echo "***** MultiAMVolOpsTest complete - exit with: ${?} *****"

    cd ${WORKSPACE}

    ps axww > source/cit/ps-out-`date +%Y%m%d%M%S`.txt

    jenkins_scripts/check_xunit_results.sh




    ######### was #9

    # Run the RestartClusterKillServices test

    # This test is disabled because it was not added and tested correctly. Please contact Aaron 
    if [ ${RESTARTCLUSTERKILLSERVICES} == 'true' ] ; then

      cd source/test/testsuites

      echo "***** RUNNING RestartClusterKillServices.ini *****"
      ./ScenarioDriverSuite.py -q ./RestartClusterKillServices.ini -d dummy --verbose
      echo "***** RestartClusterKillServices complete - exit with: ${?} *****"

      cd ${WORKSPACE}
      jenkins_scripts/check_xunit_results.sh
      echo "***** check_xunit_results exit with: ${?} *****"
    fi






}


function run_node_cleanup
{
   message "DDDD Run post build node_cleanup here"

   sleep 10

   exit $1
}


function run_coroner
{
   message "DDDD Run coroner here"

   node_cleanup $1
}


startup
configure_cache
clean_up_environment

build_fds
if [[ $? -ne 0 ]]
then
    message "Build failure detected" 
    run_coroner 1
fi


cache_report

run_node_cleanup 0
