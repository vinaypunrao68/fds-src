#!/bin/bash

usage() {
cat <<EOF

Creates graphs for all .stat files in specified directory (-d option).
Optionally, allows to specify a filter string to only plot files that
contain filter string in their name. The output are pdf files in 
'pdf' directory inside -d directory

usage: $0 options
OPTIONS:
 -h  this message
 -d  directory that contains .stat files
 -f  filter string (optional) only consider names that include this string
Requires Gnuplot >= 4.4
EOF
}

DIR=
STR=

while getopts "hd:f:" OPTION
do
  case $OPTION in
    h)
      usage
      exit 1
      ;;
    d)
      DIR=$OPTARG
      ;;
    f) 
      STR=$OPTARG
      ;;
    ?)
      usage
      exit 1
      ;;
  esac
done

### check args ######

if [ -z "$DIR" ]; then
  echo "plot-all-stats: must specify directory that contains <*.stat> files (-d option)"
  usage
  exit 1
fi

for file in $DIR/*$STR*
do
  if [[ -s $file ]] && 
     [[ ! -d $file ]]; then
    ext="${file##*.}"
    if [ $ext == "stat" ]; then 
      echo "Will plot stats for file:  $file"
      ./plot-stats.sh -s $file -p iops
      ./plot-stats.sh -s $file -p lat
    fi 
  fi
done


