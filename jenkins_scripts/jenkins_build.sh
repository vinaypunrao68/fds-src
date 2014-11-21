#!/bin/bash -le

DESIRED_SHM_SIZE="3G"      # In gigs, in full Gigs only.

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
   fi

   # Configure the CCACHE size (lives in ${CCACHE_DIR} on Jenkins boxes)
   # This needs to be a bit smaller than ${DESIRED_SHM_SIZE}
   ccache -M 2.8G
   ccache -z
}

function clean_up_environment
{
   # Make sure bare_am is stopped before we start
   killall -9 bare_am || true

   # Stop redis
   message "STOPPING redis"
   sudo source/tools/redis.sh stop

   # Clean remove the old /fds
   message "CLEANUP EXISTING /fds and /dev/shm"
   rm -rf /fds || true
   rm -rf /dev/shm/0x*
}

function build_it
{
    message "RUNNING DEVSETUP"
    make devsetup

    message "BUILDING DEPENDENCIES"
    jenkins_scripts/build-base-deps.py


    message "BUILDING FORMATION PLATFORM"
    jenkins_scripts/build-fds.py

}

function cache_report
{
   message "CCACHE POSTBUILD STATISTICS"
   ccache -s
}

startup
configure_cache
clean_up_environment
build_it
cache_report
