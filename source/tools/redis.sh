#!/bin/bash
########################################################################
# setup correct dirs
SOURCE="${BASH_SOURCE[0]}"
# resolve $SOURCE until the file is no longer a symlink
while [ -h "${SOURCE}" ]; do
    LOCATIONDIR="$(cd -P "$(dirname "${SOURCE}")" && pwd)"
    SOURCE="$(readlink "${SOURCE}")"
    [[ ${SOURCE} != /* ]] && SOURCE="${LOCATIONDIR}/${SOURCE}"
done
LOCATIONDIR="$(cd -P "$(dirname "${SOURCE}" )" && pwd)"

# one level up
TOPDIR="$(cd -P ${LOCATIONDIR}/.. && pwd)"
FDSROOT=/fds
REDISPORT=6379

# check if the script is located in the code base OR install dir
if [[ ${LOCATIONDIR}  =~ "/source/" ]] ; then
    # run from the code base
    TOOLSDIR="${LOCATIONDIR}"
    CONFIGDIR=${TOPDIR}/config/etc
    BINDIR="${TOPDIR}/Build/linux-x86_64.debug/bin"
else
    # run from the install dir
    if [[ -d ${TOPDIR}/tools ]]; then
        TOOLSDIR=${TOPDIR}/tools
    else
        TOOLSDIR=${TOPDIR}/sbin
    fi
    CONFIGDIR=${TOPDIR}/etc    
    BINDIR=${TOPDIR}/bin
fi
########################################################################
if [[ -f ${TOOLSDIR}/loghelper.sh ]]; then
    source ${TOOLSDIR}/loghelper.sh
elif [[ -f /usr/include/pkghelper/loghelper.sh ]]; then
    source /usr/include/pkghelper/loghelper.sh
else
function log() {
    echo "$@" 
}

function logwarn() {
    echo "[WARN] : $@"
}

function loginfo() {
    echo "[INFO] : $@"
}

function logerror() {
    echo "[ERROR] : $@"
}
logwarn "unable to locate loghelper.sh"

fi

if [[ -n $(declare -F init_loghelper) ]]; then
    init_loghelper /tmp/redis_tool.log
fi

versinfo=()

function usageRedis() {
    log "usage: $(basename $0) [cmd]"
    log " cmd : "
    log "   - start   : start the app " 
    log "   - stop    : stop  the app " 
    log "   - restart : stop & start  the app " 
    log "   - status  : print status of the apps"
    log "   - clean   : remove the data"
    log ""
    exit 0
}

function makeRedisDirs() {
    if [[ ! -f /fds/var/redis ]] ; then
        mkdir -p /fds/var/redis
    fi

    if [[ ! -f /fds/var/run/redis ]] ; then
        mkdir -p /fds/var/redis/run
    fi
    
    if [[ ! -f /fds/var/logs/redis ]] ; then
        mkdir -p /fds/var/logs/redis
    fi
}

# load ports from configs
function init() {
    local port=$(getRedisPort)
    if [[ -n $port ]]; then
        REDISPORT=$port
    fi
    makeRedisDirs

    # get the redis version
    versinfo=($(redis-server -v | awk '{print $3}' |cut -b 3-|sed -e 's/\./ /g'))
    #echo ${versinfo[@]}
}

function getRedisConfigFile() {
    echo -n ${CONFIGDIR}/redis.conf
}

function getRedisPort() {
    echo -n $(grep port $(getRedisConfigFile) | awk '{print $2}')
}

function getRedisPid() {
    local pid
    pid=$(ps aux | grep "redis-server $(getRedisConfigFile)" | grep -v grep | awk '{print $2}')
    if [[ -z $pid ]] ; then 
        pid=$(ps aux | grep "redis-server.*:${REDISPORT}" | grep -v grep | awk '{print $2}')
    fi
    if [[ -z $pid ]] ; then 
        pid=$(pgrep redis-server)
    fi
    
    echo $pid    
}

function isRunning() {
    local pid=$(getRedisPid)    
    if [[ -n $pid ]] ; then return 0 ; else return 1 ; fi
}

function startRedis() {
    if (isRunning) ; then
        local pid=$(getRedisPid)
        logwarn "redis [pid=$pid] is already running @ [port:${REDISPORT}]"
    else
        loginfo "starting redis @ [port:${REDISPORT}]"
        redis-server $(getRedisConfigFile)
    fi
    
}

function statusRedis() {
    if (isRunning) ; then
        local pid=$(getRedisPid)
        loginfo "redis [pid=$pid] running @ [port:${REDISPORT}]"
    else
        logwarn "redis is NOT running @ [port:${REDISPORT}]"
    fi
}

function stopRedis() {
    if (isRunning) ; then            
        local pid=$(getRedisPid)
        loginfo "shutting down redis [pid=$pid] @ [port:${REDISPORT}]"
        echo "shutdown" | redis-cli -p ${REDISPORT}
    else
        logwarn "redis is NOT running @ [port:${REDISPORT}]"
    fi
}

function cleanRedis() {
    if (isRunning) ; then
        logwarn "cleaning redis @ [port:${REDISPORT}]"
        echo "FLUSHALL" | redis-cli -p $REDISPORT >/dev/null
        echo "BGREWRITEAOF" | redis-cli -p $REDISPORT >/dev/null
    else
        logwarn "redis is NOT running @ [port:${REDISPORT}]"
    fi
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
                usageRedis
            fi
            FDSROOT=${args[$n]}
            ;;
        -i|--from-install-dir)
            run_from_install_dir=1
            ;;
        -h|--help)
            usageRedis
            ;;
        *)
            if [[ -z $CMD ]] ; then CMD=$opt
            elif [[ -z $APP ]] ; then APP=$opt
            else
                logerror "unknown option [$opt] "
                usageRedis
            fi
    esac
done

# check the values
CMD=${CMD:="status"}

case "${CMD}" in
    start) ;;
    stop) ;;
    status) ;;
    clean) ;;
    restart) ;;
    *) usageRedis ;;
esac

init

case "${CMD}" in
    start) startRedis;;
    stop) stopRedis ;;
    status) statusRedis ;;
    clean) cleanRedis;;
    restart) 
        stopRedis
        startRedis ;;
    *) usageRedis ;;
esac
