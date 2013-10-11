#!/bin/bash
#
# Plots IOPS or latency over time
#

usage()
{
cat <<EOF

Plots a graph in pdf format based on data in -s .stat file.
'-p iops' plots IOPS as function of time
'-p lat' plots Latency as function of time
The output pdf is in 'pdfs' directory inside the directory
that holds .stat file.

Usage: $0 options 
OPTIONS:
  -h  This message
  -s  Path to *stat file
  -p  iops|lat [default: iops]
      (will plot iops or latency over time)

Requires Gnuplot >= 4.4
EOF
}

STATF=
PLOTWHAT=iops

while getopts "hs:p:" OPTION
do
  case $OPTION in 
    h)
        usage
        exit 1
        ;;
    s)
        STATF=$OPTARG
        ;;
    p)  
        PLOTWHAT=$OPTARG
        ;;
    ?)
        usage
        exit 1
        ;;
    esac
done

if [ -z "$STATF" ]; then
  echo "graph-stats: You must specify .stat file"
  usage
  exit 1
fi

if [[ ! -e "$STATF" ]]; then
  echo "graph-stats: $STATF does not exist"
  exit
fi

if [[ ! -s "$STATF" ]]; then
  echo "graph-stats: $STATF is empty"
  exit
fi

if [ -z "$PLOTWHAT" ]; then
  echo "graph-stats: You must specify what to plot (iops or latency)"
  usage
  exit 1
fi

if [ "$PLOTWHAT" != "iops" ] &&
   [ "$PLOTWHAT" != "lat" ]; then
  echo "graph-stats: you must specify either iops or lat for graph type"
  usage
  exit 1
fi

STATFNAME=$(basename "$STATF")
PREFIX="${STATFNAME%.*}"

DIR=$(dirname "$STATF")/"pdfs"
OUTNAME=$PREFIX"-"$PLOTWHAT
PLTFILE=$DIR/$OUTNAME".plt"
EPSNAME=$DIR/$OUTNAME".eps"
PDFNAME=$DIR/$OUTNAME".pdf"

mkdir -p $DIR
echo "Will create $PDFNAME file in directory $DIR"

# get the earliest timestamp in file
timestamp="`awk -F"," 'BEGIN{min=999999999999} $3 < min {min=$3} END{print min}' $STATF`"

# Find all volume ids and create separate temp stat files for each volume
# we will use these temp files for plotting

vols=`cat $STATF | cut -f 2 -d ',' | sort | uniq `
vol_array=
echo "Will output stats for volumes:"
for vol in $vols
do
  echo "volume $vol"
  # remove .tmp file first in case it's there 
  TMP_VOL_FILE=$DIR/$PREFIX"-v"$vol".tmp"
  rm -f $TMP_VOL_FILE
  # create new .tmp file that filters stats from this particular volume
  awk -F"," '$2=='$vol'  {print}' $STATF >> $TMP_VOL_FILE 
  vol_array="$vol_array $vol"
done


################ IOPS graph ###########################
if [ "$PLOTWHAT" == "iops" ]; then
rm -f $PLTFILE

cat <<MARKER >> "$PLTFILE"
set terminal postscript color "Arial" 24
set style line 1 linetype 1 lw 3
set style line 2 linetype 2 lw 3
set style line 3 linetype 3 lw 3
set style line 4 linetype 4 lw 3
set style line 5 linetype 5 lw 3
set style line 6 linetype 6 lw 3
set title "IOPS"
set xlabel "Time [min]" font "Arial,24"
set key right top

set datafile separator ","

set out "$EPSNAME"
set yrange [0:*]
set ylabel "IOPS" font "Arial,24"
file(vol) = sprintf("%s/%s-v%s.tmp","$DIR","$PREFIX",vol);
plot for [vol in "$vol_array"] file(vol) using ((\$3 - $timestamp)/60):4 w lp title vol

MARKER

fi #iops
############### latency graph ##########################
if [ "$PLOTWHAT" == "lat" ]; then
rm -f $PLTFILE

cat <<MARKER >> "$PLTFILE"
set terminal postscript color "Arial" 24
set style line 1 linetype 1 lw 3
set style line 2 linetype 2 lw 3
set style line 3 linetype 3 lw 3
set style line 4 linetype 4 lw 3
set style line 5 linetype 5 lw 3
set style line 6 linetype 6 lw 3
set title "Latency"
set xlabel "Time [min]" font "Arial,24"
set key right top

set datafile separator ","

set out "$EPSNAME"
set yrange [0:*]
set ylabel "Latency [ms]" font "Arial,24"
file(vol) = sprintf("%s/%s-v%s.tmp", "$DIR", "$PREFIX", vol);
plot for [vol in "$vol_array"] file(vol) using ((\$3 - $timestamp)/60):(\$5/1000) w lp title vol

MARKER
fi #latency

#### make graph #####
if [[ -e $PLTFILE ]]; then
gnuplot $PLTFILE
sleep 2
ps2pdf $EPSNAME $PDFNAME
sleep 1
fi

#cleanup
if [[ -e $PLTFILE ]]; then
rm -f $EPSNAME
rm -f $PLTFILE
fi

exit
for vol in $vols
do
  tmpfile=$DIR/$PREFIX"-v"$vol".tmp"
  if [[ -e $tmpfile ]]; then
    rm -f $tmpfile
  fi
done



