#!/bin/bash

AB_DIR=/root/apache_bench/AB
BLOCK_DIR=/home/monchier/FDS/master/fds-src/source/tools

# Variables
applist="pm om dm sm am"
declare -A apps

apps["om"]="orchMgr"
apps["am"]="AMAgent"
apps["sm"]="StorMgr"
apps["dm"]="DataMgr"
apps["pm"]="platformd"

RESULTS=/home/monchier/results
if [ ! -e $RESULTS ]
then
	mkdir -p $RESULTS
fi

function getAppPid() {
    local name=$1
    local pid

    if [[ $name == "om" ]]; then
        pid=$(pgrep orchMgr)
        if [[ -n $pid ]] ; then
            pid=$(pgrep -P $pid 2>/dev/null)
        fi
    elif [[ $name == "am" ]]; then
        pid=$(pgrep AMAgent)
        if [[ -n $pid ]] ; then
            pid=$(pgrep -P $pid 2>/dev/null)
        fi
    else
        pid=$(pgrep ${apps[$name]})
    fi
    echo -n $pid
}

function run_stat_collectors {
    NAME=$1
	collectl -P --all > $RESULTS/collectl.$NAME &
    echo "PIDMAP" > $RESULTS/map.$NAME
    PIDS=""
    for a in $applist
    do
        _PID=$(getAppPid $a)
        echo "Map: $a --> $_PID" >> $RESULTS/map.$NAME
        PIDS="$PIDS""p$_PID,"
    done
    #collectl  -sZ -i1:1 --procfilt $PIDS > $RESULTS/collectl.procs.$NAME &
    iostat -p -d 1 -k  > $RESULTS/iostat.$NAME &
    top -p `echo $PIDS | sed 's/p//g'| sed 's/,$//'` -b > $RESULTS/top.$NAME &
}

function shutdown_stat_collectors {
    pkill collectl
    pkill iostat
    pkill top
}

function exp {
	TYPE=$1
	THREADS=$2
	N=$3
    FILES_NUM=$4
    NCLIENTS=$5
    NVOLS=$6
	NAME="$TYPE:$THREADS:$N:$FILES_NUM:$NVOLS"
	#collectl -P --all > $RESULTS/collectl.$NAME &
	#collectl -P --all --procfilt > $RESULTS/collectl.$NAME &
    run_stat_collectors $NAME
	sleep 1
    if [ $NCLIENTS -eq 2 ]
    then
	    ./traffic_gen.py -t $THREADS -n $N -T $TYPE -F $FILES_NUM -v $NVOLS 2>&1 | tee $RESULTS/out.2.$NAME &
    fi
	./traffic_gen.py -t $THREADS -n $N -T $TYPE -F $FILES_NUM -v $NVOLS 2>&1 | tee $RESULTS/out.$NAME
	sleep 1
	#ID=`python cli.py -c svclist | grep sm | awk -F ":" '{print $1}'`
    #echo "ID=$ID"
	#sleep 1
	#python cli.py -c "counters $ID sm" | tee $RESULTS/counters.$NAME
	#sleep 1
	#python cli.py -c "reset-counters $ID sm"
	#sleep 1
	#pkill collectl
    shutdown_stat_collectors
    rm -fr /tmp/fdstrgen*
}

# f_num=10000
# 
# for v in 10
# do
#     for t in "PUT"
#     do
#     	for th in 1 
#     	do
#     		for n in 100000
#     		do
#     		echo "Experiment: $t $th $n $f_num 1 $v"
#     		exp $t $th $n $f_num 1 $v
#     		sleep 1		
#     		done
#     	done
#     done
# done
# 
# for v in 1 2 3 4 5 10
# do
#     for t in "GET" 
#     do
#     	for th in 1 2 3 4 5 6 7 8 9 10 15
#     	do
#     		for n in 10000
#     		do
#     		echo "Experiment: $t $th $n $f_num 1 $v"
#     		exp $t $th $n $f_num 1 $v
#     		sleep 1		
#     		done
#     	done
#     done
# done

# for t in "7030" 
# do
# 	for th in 1 2 3 4 5 6 7 8 9 10 15 20 25 30
# 	do
# 		for n in 10000
# 		do
# 		echo "Experiment: $t $th $n $f_num 1"
# 		exp $t $th $n $f_num 1
# 		sleep 1		
# 		done
# 	done
# done


function do_ab_put {
    N=$1
    C=$2
    V=$3
	NAME="AB_PUT:$N:$C:$V"
    run_stat_collectors $NAME
	sleep 1
    ###### Workload
    pushd $AB_DIR
    ./runme.sh -E -n $N -c $C -p 5MB_rand.0 -b 4096 -R ab_blobs.txt http://localhost:8000/volume$V/ 2>&1 | tee $RESULTS/out.$NAME
    popd
    ###### --------
	#sleep 1
	#ID=`python cli.py -c svclist | grep sm | awk -F ":" '{print $1}'`
    #echo "ID=$ID"
	#sleep 1
	#python cli.py -c "counters $ID sm" | tee $RESULTS/counters.$NAME
	#sleep 1
	#python cli.py -c "reset-counters $ID sm"
	sleep 1
	#pkill collectl
    shutdown_stat_collectors
}

function do_ab_get {
    N=$1
    C=$2
    V=$3
	NAME="AB_GET:$N:$C:$V"
    run_stat_collectors $NAME
	sleep 1
    ###### Workload
    pushd $AB_DIR
    ./runme.sh -E -n $N -c $C http://localhost:8000/volume$V/ 2>&1 | tee $RESULTS/out.$NAME
    popd
    ###### --------
	#sleep 1
	#ID=`python cli.py -c svclist | grep sm | awk -F ":" '{print $1}'`
    #echo "ID=$ID"
	#sleep 1
	#python cli.py -c "counters $ID sm" | tee $RESULTS/counters.$NAME
	#sleep 1
	#python cli.py -c "reset-counters $ID sm"
	sleep 1
	#pkill collectl
    shutdown_stat_collectors
}


# for c in 1 
# do
# 	for n in 100000
# 	do
# 	    echo "Experiment: ab_put $n $c 0"
#         do_ab_put $n $c 
#     done
# done
# for c in 1 2 3 4 5 6 7 8 9 10 15 20 25 30
# do
# 	for n in 10000
# 	do
# 	    echo "Experiment: ab_put $n $c 0"
#         do_ab_get $n $c 
#     done
# done

function do_block {
    TYPE=$1
    N=$2
    C=$3
    NVOLS=$4
	NAME="$TYPE:$N:$C:$NVOLS"
    run_stat_collectors $NAME
	sleep 1
    ###### Workload
    pushd $BLOCK_DIR
    pids=""
    for i in `seq 1 $NVOLS`
    do
        let j=$i-1
        if [ $TYPE == "PUT" ] 
        then
            ./run-delay-workloads.sh -p "poc-demo-write" -w "sample_workloads/seq_write.sh:/dev/nbd$j" 2>&1 | tee $RESULTS/out.$i.$NAME &
        else
            ./run-delay-workloads.sh -p "poc-demo-read" -w "sample_workloads/seq_read.sh:/dev/nbd$j" 2>&1 | tee $RESULTS/out.$i.$NAME &
        fi
        pids="$pids $!"
    done    
    echo "---> Waiting on $pids"
    wait $pids
    popd
    ###### --------
	#sleep 1
	#ID=`python cli.py -c svclist | grep sm | awk -F ":" '{print $1}'`
    #echo "ID=$ID"
	#sleep 1
	#python cli.py -c "counters $ID sm" | tee $RESULTS/counters.$NAME
	#sleep 1
	#python cli.py -c "reset-counters $ID sm"
	sleep 1
	#pkill collectl
    shutdown_stat_collectors
}

for t in "PUT" "GET"
do
	for v in 1 
	do
	    for th in 1
	    do
	    	for n in 1
	    	do
	    	echo "Experiment: $t $n $th $v"
            do_block $t $n $th $v
	    	sleep 1		
	    	done
	    done
   done
done



