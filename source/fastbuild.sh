#!/bin/bash

dir_list="fdsp apis include osdep net-service umod-lib util lib fds-probe nginx-driver persistent_layer platform orch_mgr data-mgr access-mgr stor-mgr tools java"

not_built="unit-test checker"

# don't set to less than 2, this is the number of retries per directory
RETRY_COUNT=3 

cpu_count=`cat /proc/cpuinfo | grep -c "^processor"`
mem_gigs=`free --si -g | grep "Mem:"|awk '{ print $2 }'`

case ${mem_gigs} in
   1|2) cpu_count=1;
        ;;
   3) cpu_count=2;
      ;;
   4) cpu_count=3;
      ;;
   5) cpu_count=4;
      ;;
   *) cpu_count=$((cpu_count+1))
      ;;
esac

if [ "$1" = "-j" ]; then
    echo $2
    cpu_count=$2
fi

echo "Starting build with -j ${cpu_count}, you have ${mem_gigs}G of memory in your system"

for dir in ${dir_list}
do
   cd ${dir}
   try_count=0

   while [[ try_count -ne ${RETRY_COUNT} ]]
   do
      make -j ${cpu_count}
        #make CCACHE=1 -j ${cpu_count}       ## investigate further

      [[ $? -eq 0 ]] && break;

      try_count=$((try_count+1))
   done

   cd - > /dev/null

   [[ ${try_count} -eq ${RETRY_COUNT} ]] && exit 99;       # had 3 failures so bail
done

# This  makes one final pass to ensure everything is done
for dir in ${dir_list}
do
   cd ${dir}
   make
   cd - > /dev/null
done
