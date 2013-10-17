#!/bin/bash
#
# ./run-experiment -w "workload file:min_iops,max_iops,prio"
# 
# For each -w option, will create policy with given min_iops, max_iops, prio, create volume with
# this policy, and attach it to local node
#
# Then, will run workloads specified in each 'workload file'.
#
# During cleanup, will remove all created volumes and policies
#

SCRIPT_DIR="`pwd`"
RESULT_DIR=$SCRIPT_DIR/results
BIN_DIR=../Build/linux-i686/bin

usage() {
cat <<EOF

Runs workloads on local SH that access FDS via block API.
Assumes that OM, SM(s), and DM(s) are up.
Will start ubd process and insmod fbd.ko
For each workload specified with -w option, will create a policy
with given (min_iops, max_iops, prio) values, create a volume with 
a given policy, and attach a volume to localhost-sh. That workload
will access /dev/fbdN device (first -w workload will access /deb/fbd0, 
second -w workload will access /dev/fbd1 and so on). 

usage: $0 options
OPTIONS:
 -h  this message
 -p  prefix (optional) - all resuts files will be prefixed with this string
 -w  workload = "workload_file:min_iops,max_iops,prio"
     workload_file is a shell script that will call a workload generator.
EOF
}

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
  WORKLOADS="$WORKLOADS $wf"

  policy=`echo $wdesc | cut -f 2 -d ":"`
  id=$NEXT_ID
  policy="$id,$policy"
  POLICIES="$POLICIES $policy"

  NEXT_ID=`expr $NEXT_ID + 1`

  # check if policy is correctly specified # 
  min_iops=`echo $policy | cut -f 2 -d ","`
  if [ -z $min_iops ]; then
    echo "run-experiment: must specify min_iops for workload $wf"
    usage
    exit 1
  fi
  max_iops=`echo $policy | cut -f 3 -d ","`
  if [ -z $max_iops ]; then
    echo "run-experiment: must specify max_iops for workload $wf"
    usage
    exit 1
  fi
  prio=`echo $policy | cut -f 4 -d ","`
  if [ -z $prio ]; then
    echo "run-experiment: must specify priority 'prio' for workload $wf"
    usage
    exit 1
  fi

  echo "workload $wf, vol & policy id $id, min_iops $min_iops, max_iops $max_iops, priority $prio"
}

WORKLOADS=
POLICIES=
START_ID=3000
NEXT_ID=$START_ID
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

# we will create a volume for each -w option, we assume that no volumes currently exist #
# check that there are not /dev/fbd? devices #
devfiles=(/dev/fbd?)
if [ -e "${devfiles[0]}" ]; then
  echo "run-experiment: expects that there are no /dev/fbd? files"
  exit 1
fi

# remove all .stat files from fds_client/stats dir, so that we know that
# after the end of experiment, all .stat files belong to this experiment
rm -f $BIN_DIR/stats/*

# make sure results dir exists
mkdir -p $RESULT_DIR

cleanup() {
  for policy in $POLICIES
  do
    pid=`echo $policy | cut -f 1 -d ","`
    pname="Policy_$pid"
    volid=$pid
    volname="Volume_$volid"
    pushd .; cd $BIN_DIR
      echo "Detaching volume $volname with id $volid" 
      ./fdscli --volume-detach $volname -i $volid -n localhost-sh
      sleep 1
      echo "Deleting volume $volname with id $volid"
      ./fdscli --volume-delete $volname -i $volid
      sleep 1
      echo "Deleting policy $pname with id $pid"
      ./fdscli --policy-delete $pname -p $pid
      sleep 1
    popd
  done
}

### start ubd in background #####
insmod ../stor_hvisor/fbd.ko
pushd .; cd $BIN_DIR
  ./ubd &
popd
sleep 10

#####  create policies, and volume for each policy  ######
devix=1
for policy in $POLICIES
do
  # create policy #
  pid=`echo $policy | cut -f 1 -d ","`
  pname="Policy_$pid"
  min_iops=`echo $policy | cut -f 2 -d ","`
  max_iops=`echo $policy | cut -f 3 -d ","`
  prio=`echo $policy | cut -f 4 -d ","`
  echo "Will create policy: id $pid, name $pname, min_iops $min_iops, max_iops $max_iops, priority $prio"
  pushd .; cd $BIN_DIR
  ./fdscli --policy-create $pname -p $pid -g $min_iops -m $max_iops -r $prio
  popd
  sleep 1

  # create volume #
  volid=$pid
  volname="Volume_$volid"
  size=1000000000
  echo "Will create volume: id $volid, name $volname, size $size, policy id $pid"
  pushd .; cd $BIN_DIR
  ./fdscli --volume-create $volname -i $volid -s $size -p $pid
  popd
  sleep 2

  # attach volume to localhost #
  echo "Will attach volume: id $volid, name $volname, node localhost-sh"
  pushd .; cd $BIN_DIR
  ./fdscli --volume-attach $volname -i $volid -n localhost-sh
  popd
  # TODO: ability to attach to different storage nodes, need to add node to workload descriptor # 
  sleep 1


  # this should create /dev/fbd$devix device, check if it exists 
  devfile=/dev/fbd$devix
  if [ ! -e $devfile ]; then
    echo "run-experiment: Unexpected error, $devfile should exist"
    cleanup 
    exit 1
  fi 
  devix=`expr $devix + 1` 
  sleep 1

done

sleep 10

#### start workloads #####
devix=1
wpids=
for workload in $WORKLOADS
do
  devfile=/dev/fbd$devix
  volid=`expr $START_ID + $devix`
  echo "Will run workload $workload on device $devfile"
  ./$workload $RESULT_DIR $PREFIX $devfile $volid &
  wpid=$!
  wpids="$wpids $wpid"
  devix=`expr $devix + 1`
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
# NOTE: need to turn on stats dump by calling SH->vol_table->startPerfStats() that
# are currently called only from StorHvisorNet.cpp::unitTest() 
#
for file in $BIN_DIR/stats/* 
do
  fname=$(basename $file)
  cp $file $RESULT_DIR/$PREFIX"_SH_"$fname
done
./plot-all-stats.sh -d $RESULT_DIR -f $PREFIX"_SH_"

#### cleanup ##########
cleanup
pkill ubd
sleep 5
rmmod fbd.ko
















