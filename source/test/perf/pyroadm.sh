#!/bin/bash

OPTIND=1         # Reset in case getopts has been used previously in the shell.

start=0
check=0
down=0

function show_help {
    echo "Usage: ./prog [-s/-c/-d] hostname"
}

while getopts "h?scdr" opt; do
    case "$opt" in
    h|\?)
        show_help
        exit 0
        ;;
    s)  start=1
        ;;
    c)  check=1
        ;;
    d)  down=1
        ;;
    esac
done

shift $((OPTIND-1))

echo "start=$start, check=$check, down=$down, Leftovers: $@"


PYRO_IP="10.1.10.139"
PYRO_PORT=47672
REMOTE=$PYRO_IP

loc_ip=`ifconfig | grep 10.1.10 | awk '{print $2}' |  awk -F ":" '{print $2}'`
cmd="ifconfig | grep 10.1.10 | awk '{print \$2}'|  awk -F ":" '{print \$2}'"
rem_ip=`ssh $REMOTE $cmd`
rem_hostname=`ssh $REMOTE hostname`
echo "loc_ip: $loc_ip, rem_ip: $rem_ip"


shift $((OPTIND-1))

echo "start=$start, check=$check, down=$down, Leftovers: $@"


PYRO_IP="10.1.10.139"
PYRO_PORT=47672
REMOTE=$PYRO_IP

loc_ip=`ifconfig | grep 10.1.10 | awk '{print $2}' |  awk -F ":" '{print $2}'`
cmd="ifconfig | grep 10.1.10 | awk '{print \$2}'|  awk -F ":" '{print \$2}'"
rem_ip=`ssh $REMOTE $cmd`
rem_hostname=`ssh $REMOTE hostname`
echo "loc_ip: $loc_ip, rem_ip: $rem_ip"

if [ $check -eq 1 ]; then
    ns_up=`ssh $PYRO_IP 'ps aux|grep Pyro4.naming| grep -v grep' | wc -l`
    cmd_rem_up=`ssh $REMOTE 'ps aux|grep cmd-server.py |grep -v grep' | wc -l`
    cmd_loc_up=`ps aux|grep cmd-server.py |grep -v grep | wc -l`
    echo "ns: $ns_up, cmd_rem: $cmd_rem_up, cmd_loc: $cmd_loc_up"
    up=0
    if [ $ns_up -eq 1 ] && [ $cmd_rem_up -eq 1 ] && [ $cmd_loc_up -eq 1 ] ; then
        up=1
    fi
    echo "all: $up"
fi

if [ $down -eq 1 ]; then
    ns_pids=`ssh $PYRO_IP 'ps aux|grep Pyro4.naming| grep -v grep' | awk '{print $2}'`
    cmd_rem_pids=`ssh $REMOTE 'ps aux|grep cmd-server.py |grep -v grep'  | awk '{print $2}'`
    cmd_loc_pids=`ps aux|grep cmd-server.py |grep -v grep  | awk '{print $2}'`
    echo "ns: $ns_pid, cmd_rem: $cmd_rem_pid, cmd_loc: $cmd_loc_pid"
    for p in $ns_pids ; do
        ssh $PYRO_IP "kill -9 $p"
    done
    for p in $cmd_rem_pids ; do
        ssh $REMOTE "kill -9 $p"
    done
    for p in $cmd_loc_pids ; do
        kill -9 $p
    done
fi

if [ $start -eq 1 ]; then
    ssh $REMOTE "nohup python -Wignore -m Pyro4.naming --host $PYRO_IP --port $PYRO_PORT 2> pyro_ns.err > pyro_ns.out &"
    sleep 2
    scp cmd-server.py $REMOTE:
    ssh $REMOTE "nohup python cmd-server.py -d $rem_hostname -n $PYRO_IP -p $PYRO_PORT -s $rem_ip 2> cmd_server.err > cmd_server.out &"
    nohup python cmd-server.py -d $HOSTNAME -n $PYRO_IP -p $PYRO_PORT -s $loc_ip 2> cmd_server.err > cmd_server.out &
fi

