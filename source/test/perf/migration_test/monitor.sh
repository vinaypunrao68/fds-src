#!/bin/bash

out=$1

sm_pid=`pidof StorMgr`

pids=""
top -b -p $sm_pid > $out.top &
pids="$pids $!"
iostat -kxd 1 > $out.iostat &
pids="$pids $!"
wait $pids
