#!/bin/bash
#
# ./run-experiment -w "workload file:/dev/fbdN"
#
# Then, will run workloads specified in each 'workload file'
# against spevified /dev/fbdN
#
# During cleanup, will remove all created volumes and policies
#

SCRIPT_DIR="`pwd`"
RESULT_DIR=$SCRIPT_DIR/results
BIN_DIR=../Build/linux-i686/bin

usage() {
cat <<EOF

Runs workloads on local SH that access FDS via block API.
Assumes that SH, OM, SM(s), and DM(s) are up and policies 
and volumes are created.

Each workload specified with -w option will access /dev/fbdX device 
on this node.

usage: $0 options
OPTIONS:
 -h  this message
 -p  prefix (optional) - all resuts files will be prefixed with this string
 -w  workload = "workload_file:/dev/fbdX"
     workload_file is a shell script that will call a workload generator.
EOF
}

# check if wdesc specified correctly 
parse_wdesc() {
  wdesc=$1

  wf=`echo $wdesc | cut -f 1 -d ":"`
  if [ -z $wf ]; then
    echo "run-experiment: must specify workload file with -w option"
    usage
    exit 1
  fi
  if [ ! -e $wf ]; then
    echo "run-experiment: workload file $wf does not exist"
    exit 1
  fi
  WORKLOADS="$WORKLOADS $wdesc"

  device=`echo $wdesc | cut -f 2 -d ":"`

  # check that device exists
  if [ ! -e $device ]; then
    echo "Device $device does not exist"
    exit 1
  fi

  echo "workload $wf will access device $device"
}

START_ID=0
WORKLOADS=
PREFIX=

while getopts "hw:p:" OPTION
do
  case $OPTION in
    h)
        usage
        exit 1
        ;;
    p)
        PREFIX=$OPTARG
        ;;
    w)
        parse_wdesc $OPTARG
        ;;
    ?)
        usage
        exit
        ;; 
  esac
done

if [[ -z $WORKLOADS ]]; then
  echo "run-experiment: must specify at least one workload to run (-w option)" 
  usage
  exit 1
fi

# remove all .stat files from fds_client/stats dir, so that we know that
# after the end of experiment, all .stat files belong to this experiment
#rm -f $BIN_DIR/stats/*

# make sure results dir exists
mkdir -p $RESULT_DIR

#### start workloads #####
ix=1
wpids=
for wdesc in $WORKLOADS
do
  workload=`echo $wdesc | cut -f 1 -d ":"`
  device=`echo $wdesc | cut -f 2 -d ":"`
  wid=`expr $START_ID + $ix`

  echo "Will run workload $workload on device $device"
  ./$workload $RESULT_DIR $PREFIX $device $wid &
  wpid=$!
  wpids="$wpids $wpid"
  ix=`expr $ix + 1`

  sleep 15
done

echo "Waiting for workloads to finish"

for wpid in $wpids
do
  wait $wpid
done
echo "All workloads finished running, will cleanup"

#### create graphs ######

# concatenate all files -- a hack because plot-all-stats will plot one graph 
# per .stat file, so if we want results of all volumes on one graph, need to 
# make one .stat file. If we run workload generator other than pharos, we need to 
# enable stat output in SH, and pass a created stat file into plot-all-stats to 
# plot the result 
cat $RESULT_DIR/*$PREFIX*.stat > $RESULT_DIR/$PREFIX"_allvolumess.stat"
./plot-all-stats.sh -d $RESULT_DIR -f $PREFIX

# also go to fds_client/stats and make graphs for all .stat files created
# this is useful if we run workload generator other than pharos (that does not 
# output fine-granularity stats) like dd, etc. Assumes that we cleaned up that
# directory in the beginning of the experiment, so all files in there are 
# the files from this experiment
# UNCOMMENT BELOW if you want -- but better to clean stats dir once in a while
# otherwise will take foreever to remake graphs for all (old) .stat files
#
#for file in $BIN_DIR/stats/* 
#do
#  fname=$(basename $file)
#  cp $file $RESULT_DIR/$PREFIX"_"$fname
#done
#./plot-all-stats.sh -d $RESULT_DIR -f $PREFIX"_SH_"

















