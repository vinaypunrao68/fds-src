#!/bin/bash -l

###
#  ar_wrapper.sh
#
#  This script provides a thread safe wrapper around the /usr/bin/ar command
#
#  We do this because compling with multiple threads (processes) and updating 
#  static libraries is not thread safe.  Using CCACHE exacerbates this issue by
#  not having the system spending time compiling.

###
#  Tunables
#
AR_BINARY="/usr/bin/ar"
AR_SEMAPHORE="/dev/shm/ar_wrapper_thread_safety_lock"
LOCK_EXPIRE_TIME=30            # in seconds

function remove_if_expired
{
   time_now=$(date +"%s")

   if [[ $(( time_now - birth_time )) -gt ${LOCK_EXPIRE_TIME} ]]
   then
      echo "Removing an expired ${AR_BINARY} lock semaphore (This message is informational)"
      rmdir ${AR_SEMAPHORE}
      return 0
   fi 
   return 1
}

birth_time=$(/usr/bin/stat --printf %Z ${AR_SEMAPHORE} 2>/dev/null)

# attempt to acquire the lock
while ! mkdir ${AR_SEMAPHORE} 2> /dev/null
do
   remove_if_expired && continue
   sleep 1
done

${AR_BINARY} ${@}

exit_code=$?

rmdir ${AR_SEMAPHORE}

[[ ${exit_code} -gt 0 ]] && exit ${exit_code}

exit 0
