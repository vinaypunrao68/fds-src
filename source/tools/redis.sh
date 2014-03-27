#!/bin/bash
########################################################################
# setup correct dirs
SOURCE="${BASH_SOURCE[0]}"
# resolve $SOURCE until the file is no longer a symlink
while [ -h "$SOURCE" ]; do
  DIR="$( cd -P "$( dirname "$SOURCE" )" && pwd )"
  SOURCE="$(readlink "$SOURCE")"
  [[ $SOURCE != /* ]] && SOURCE="$DIR/$SOURCE"
done

# If run from source dir
TOOLSDIR="$(cd -P "$( dirname "${SOURCE}" )" && pwd )"
SOURCEDIR="$( cd ${TOOLSDIR}/.. && pwd)"
CONFIG_ETC=${SOURCEDIR}/config/etc
BINDIR="${SOURCEDIR}/Build/linux-x86_64.debug/bin"
FDSROOT=/fds

# else run from install dir
run_from_install_dir=0

source ${TOOLSDIR}/loghelper.sh
########################################################################

PORTS=()
versinfo=()

function usageRedis() {
    log "usage: $(basename $0) [cmd] [instance]"
    log " cmd : "
    log "   - start   : start the app " 
    log "   - stop    : stop  the app " 
    log "   - restart : stop & start  the app " 
    log "   - status  : print status of the apps"
    log "   - clean   : remove the data"
    log ""
    log "instance : 1-4 [default all]"
    exit 0
}

function makeRedisDirs() {
    if [[ ! -f /fds/var/redis ]] ; then
        mkdir -p /fds/var/redis
    fi

    if [[ ! -f /fds/var/run/redis ]] ; then
        mkdir -p /fds/var/redis/run
    fi
    
    if [[ ! -f /fds/logs/redis ]] ; then
        mkdir -p /fds/logs/redis
    fi
}

# load ports from configs
function init() {
    for instance in 1 2 3 4 ; do
        PORTS[$instance]=$(getRedisPort ${instance})
    done
    makeRedisDirs

    # get the redis version
    versinfo=($(redis-server -v | awk '{print $3}' |cut -b 3-|sed -e 's/\./ /g'))
    #echo ${versinfo[@]}
}


function getRedisConfigFile() {
    local instance="$1"
    echo -n ${CONFIG_ETC}/redis${instance}.conf
}

function getRedisPort() {
    local instance="$1"
    echo -n $(grep port $(getRedisConfigFile ${instance}) | awk '{print $2}')
}

function getRedisPid() {
    local instance="$1"
    local pid
    pid=$(ps aux | grep "redis-server $(getRedisConfigFile $instance)" | grep -v grep | awk '{print $2}')
    if [[ -z $pid ]] ; then 
        pid=$(ps aux | grep "redis-server.*:${PORTS[${instance}]}" | grep -v grep | awk '{print $2}')
    fi
    if [[ -z $pid && $instance == 1 ]] ; then 
        pid=$(pgrep redis-server)
    fi
    
    echo $pid    
}

function isRunning() {
    local instance="$1"
    local pid=$(getRedisPid $instance)
    
    if [[ -n $pid ]] ; then return 0 ; else return 1 ; fi
}

function startRedis() {
    local instances="$1"
    local instance;
    for instance in ${instances} ; do
        local port=${PORTS[${instance}]}
        if (isRunning ${instance}) ; then
            local pid=$(getRedisPid ${instance})
            logwarn "redis [${instance}:pid=$pid] is already running @ [port:${PORTS[${instance}]}]"
        else
            loginfo "starting redis [${instance}] @ [port:${PORTS[${instance}]}]"
            redis-server $(getRedisConfigFile ${instance})
        fi
    done
}

function statusRedis() {
    local instances="$1"
    for instance in ${instances} ; do
        if (isRunning ${instance}) ; then
            local port=${PORTS[${instance}]}
            local pid=$(getRedisPid ${instance})
            loginfo "redis [${instance}:pid=$pid] running @ [port:${PORTS[${instance}]}]"
        else
            logwarn "redis [${instance}] is NOT running @ [port:${PORTS[${instance}]}]"
        fi
    done
}

function stopRedis() {
    local instances="$1"

    for instance in ${instances} ; do
        if (isRunning ${instance}) ; then            
            local pid=$(getRedisPid ${instance})
            loginfo "shutting down redis [${instance}:pid=$pid] @ [port:${PORTS[${instance}]}]"
            echo "shutdown" | redis-cli -p ${PORTS[${instance}]}
        else
            logwarn "redis [${instance}] is NOT running @ [port:${PORTS[${instance}]}]"
        fi
    done
}

function cleanRedis() {
    local instances="$1"
    for instance in ${instances} ; do
        if (isRunning ${instance}) ; then
            local port=${PORTS[${instance}]}
            logwarn "cleaning redis [${instance}] @ [port:${PORTS[${instance}]}]"
            echo "FLUSHALL" | redis-cli -p $port >/dev/null
            echo "BGREWRITEAOF" | redis-cli -p $port >/dev/null
        else
            logwarn "redis [${instance}] is NOT running @ [port:${PORTS[${instance}]}]"
        fi
    done
}

#################################################################################
# main
#################################################################################

# parse input
# parse out the options & leave the rest is saved
args=( "$@" )
for ((n=0; n<$#; n++ ))
do
    opt=${args[$n]}
    case $opt in
        -f|--fds-root)
            (( n++ ))
            if [[ $n -ge $# ]]; then 
                logerror "value expected for fdsroot"
                usage
            fi
            FDSROOT=${args[$n]}
            ;;
        -i|--from-install-dir)
            run_from_install_dir=1
            ;;
        -h|--help)
            usage
            ;;
        *)
            if [[ -z $CMD ]] ; then CMD=$opt
            elif [[ -z $APP ]] ; then APP=$opt
            else
                logerror "unknown option [$opt] "
                usage
            fi
    esac
done

# else run from install dir
if [ $run_from_install_dir -eq 1 ]; then
    SOURCEDIR="$( cd ${TOOLSDIR}/../.. && pwd)"
    CONFIG_ETC=${SOURCEDIR}/etc
    BINDIR=${SOURCEDIR}/bin
    FDSROOT=${SOURCEDIR}
fi

# check the values
CMD=${CMD:="status"}
APP=${APP:="all"}

case "${CMD}" in
    start) ;;
    stop) ;;
    status) ;;
    clean) ;;
    restart) ;;
    *) usageRedis ;;
esac

case "${APP}" in
    1) ;;
    2) ;;
    3) ;;
    4) ;;
    all) ;;
    *) usageRedis ;;
esac

if [[ $APP == "all" ]]; then
    APP="1 2 3 4"
fi

init

case "${CMD}" in
    start) startRedis "$APP" ;;
    stop) stopRedis  "$APP";;
    status) statusRedis "$APP" ;;
    clean) cleanRedis "$APP";;
    restart) stopRedis "$APP"
        startRedis "$APP";;
    *) usageRedis ;;
esac
