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
      echo "/dev/shm converted to ${DESIRED_SHM_SIZE}"
   fi

   # Configure the CCACHE size (lives in ${CCACHE_DIR} on Jenkins boxes)
   # This needs to be a bit smaller than ${DESIRED_SHM_SIZE}
   ccache -M 2.8G
   ccache -z
}

function clean_up_environment
{
   message "KILLING ANY RUNNING bare_am"
   pkill -9 -f bare_am || true

   message "STOPPING redis"
   sudo source/tools/redis.sh stop

   # Clean remove the old /fds
   message "CLEANUP EXISTING /fds AND /dev/shm"
   rm -rf /fds || true
   rm -rf /dev/shm/0x*
}

function build_fds
{
    message "RUNNING DEVSETUP"
    make devsetup

    message "BUILDING FORMATION PLATFORM"
		if [ ${BUILD_TYPE} -eq 'release' ] ; then
			echo "*** BUILD_TYPE: RELEASE ***"
			jenkins_scripts/build_fds.py -r
		else
			echo "*** BUILD_TYPE: DEBUG ***"
			jenkins_scripts/build_fds.py
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
