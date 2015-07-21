#!/bin/bash -le

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

DESIRED_SHM_SIZE="3G"      # In gigs, in full Gigs only.
KILL_LIST=(bare_am AmFunctionalTest NbdFunctionalTest)

export CCACHE_DIR="/dev/shm/.ccache"

function message
{
   echo "================================================================================"
   echo "$*"
   echo "================================================================================"
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
   message "CLEANUP EXISTING /fds AND /dev/shm"
   rm -rf /fds || true
   rm -rf /dev/shm/0x*
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
        jenkins_scripts/build_fds.py -r
    else
        message "BUILDING FORMATION PLATFORM - BUILD_TYPE: debug"
        if [[ ${COVERAGE} == 'true' ]]; then
            jenkins_scripts/build_fds.py --coverage
        else
            jenkins_scripts/build_fds.py
        fi
    fi
    end_time=$(date +%s)

    performance_report BUILD_FDS $(( ${end_time} - ${start_time} ))
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

function cache_report
{
   message "CCACHE POSTBUILD STATISTICS"
   ccache -s
}

startup
configure_cache
clean_up_environment
build_fds
cache_report
