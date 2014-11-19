#!/bin/bash -l

###
#  ar_wrapper.sh
#
#  This script provides a thread safe wrapper around the /usr/bin/ar command
#
#  We do this because compling with multiple threads (processes) and updating 
#  static libraries is not thread safe.  Using CCACHE exacerbates this by
#  not having the system spending time compiling.

###
#  Tunables
#
AR_BINARY="/usr/bin/ar"
AR_SEMAPHORE="/dev/shm/ar_wrapper_thread_safety_lock"
LOCK_WAIT_TIME=30                           # in seconds

start_time=$(date +"%s")

while ! mkdir ${AR_SEMAPHORE} 2> /dev/null
do
   echo "Encountered an existing ar lock semaphore"
   now=$(date +"%s")
   if [[ $(( ${start_time} + ${LOCK_WAIT_TIME} )) -lt ${now} ]]
   then
      echo "Unable to acquire the ar lock semaphore in ${LOCK_WAIT_TIME} seconds."
      exit 109        # arbitrary non zero value
   fi

   sleep 1
done

${AR_BINARY} ${@}

exit_code=$?

rmdir ${AR_SEMAPHORE}

[[ ${exit_code} -gt 0 ]] && exit ${exit_code}

exit 0
