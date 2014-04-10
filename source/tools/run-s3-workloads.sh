#!/bin/bash
#
# ./run-experiment -w "workload file:volume_name"
#
# Then, will run workloads specified in each 'workload file'
# on volume 'volume_name'
#

SCRIPT_DIR="`pwd`"
RESULT_DIR=$SCRIPT_DIR/results

usage() {
cat <<EOF

Runs workloads on local SH that access FDS via S3 API
Assumes that SH, OM, SM(s), and DM(s) are up and policies 
and volumes are created.

Each workload specified with -w option will access 'volume' volume

usage: $0 options
OPTIONS:
 -h  this message
 -p  prefix (optional) - all resuts files will be prefixed with this string
 -w  workload = "workload_file:volume"
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

  volume=`echo $wdesc | cut -f 2 -d ":"`

  echo "workload $wf will access volume $volume"
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


# make sure results dir exists
mkdir -p $RESULT_DIR

#### start workloads #####
ix=1
wpids=
for wdesc in $WORKLOADS
do
  workload=`echo $wdesc | cut -f 1 -d ":"`
  volume=`echo $wdesc | cut -f 2 -d ":"`
  wid=`expr $START_ID + $ix`

  echo "Will run workload $workload on volume $volume"
  ./$workload $RESULT_DIR $PREFIX $volume &
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
echo "All workloads finished running"

















