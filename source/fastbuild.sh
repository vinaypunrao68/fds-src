#!/bin/bash

# Keep this list sychronized with Makefile
#
dir_list="
    fdsp                                \
    apis                                \
    include                             \
    osdep                               \
    net-service                         \
    umod-lib                            \
    util                                \
    lib                                 \
    nginx-driver                        \
    persistent-layer                    \
    platform                            \
    orch-mgr                            \
    data-mgr                            \
    access-mgr                          \
    stor-mgr                            \
    tools                               \
    testlib                             \
    unit-test                           \
    checker                             \
    java
    "

echo "fastbuild.sh is deprecated, the preferred build method is:  'make fastb=1 threads=X CCACHE=1'"

sleep 5

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

# Since java built once and doesn't have rediculous dependencies, skip the final pass.
dir_list=${dir_list/java/}

for dir in ${dir_list}
do
   cd ${dir}
   make
   cd - > /dev/null
done
