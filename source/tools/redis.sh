#!/bin/bash
source ./loghelper.sh

CONFDIR=../config/etc
PORTS=()

# load ports from configs
function initPorts() {
    for instance in 1 2 3 4 ; do
        PORTS[$instance]=$(getPort ${instance})
    done
}


function usage() {
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
        mkdir -p /fds/var/run/redis
    fi
    
    if [[ ! /fds/logs/redis ]] ; then
        mkdir -p /fds/logs/redis
    fi
}


function getConfigFile() {
    local instance="$1"
    echo -n ${CONFDIR}/redis${instance}.conf
}

function getPort() {
    local instance="$1"
    echo -n $(grep port $(getConfigFile ${instance}) | awk '{print $2}')
}

function getPid() {
    local instance="$1"
    echo $(ps aux | grep "redis-server .*:${PORTS[${instance}]}" | grep -v grep | awk '{print $2}')
}

function isRunning() {
    local instance="$1"
    local pid=$(ps aux | grep "redis-server .*:${PORTS[${instance}]}" | grep -v grep | awk '{print $2}')
    
    if [[ -n $pid ]] ; then return 0 ; else return 1 ; fi
}

function start() {
    local instances="$1"
    local instance;
    for instance in ${instances} ; do
        local port=${PORTS[${instance}]}
        if (isRunning ${instance}) ; then
            local pid=$(getPid ${instance})
            logwarn "redis [${instance}:pid=$pid] is already running @ [port:${PORTS[${instance}]}]"
        else
            loginfo "starting redis [${instance}] @ [port:${PORTS[${instance}]}]"
            redis-server $(getConfigFile ${instance})
        fi
    done
}

function status() {
    local instances="$1"
    for instance in ${instances} ; do
        if (isRunning ${instance}) ; then
            local port=${PORTS[${instance}]}
            local pid=$(getPid ${instance})
            loginfo "redis [${instance}:pid=$pid] running @ [port:${PORTS[${instance}]}]"
        else
            logwarn "redis [${instance}] is NOT running @ [port:${PORTS[${instance}]}]"
        fi
    done
}

function stop() {
    local instances="$1"

    for instance in ${instances} ; do
        if (isRunning ${instance}) ; then            
            local pid=$(getPid ${instance})
            loginfo "shutting down redis [${instance}:pid=$pid] @ [port:${PORTS[${instance}]}]"
            echo "shutdown" | redis-cli -p ${PORTS[${instance}]}
        else
            logwarn "redis [${instance}] is NOT running @ [port:${PORTS[${instance}]}]"
        fi
    done
}

function clean() {
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

CMD=$1
APP=$2

CMD=${CMD:="status"}
APP=${APP:="all"}

case "${CMD}" in
    start) ;;
    stop) ;;
    status) ;;
    clean) ;;
    restart) ;;
    *) usage ;;
esac

case "${APP}" in
    1) ;;
    2) ;;
    3) ;;
    4) ;;
    all) ;;
    *) usage ;;
esac

if [[ $APP == "all" ]]; then
    APP="1 2 3 4"
fi

makeRedisDirs
initPorts

case "${CMD}" in
    start) start "$APP" ;;
    stop) stop  "$APP";;
    status) status "$APP" ;;
    clean) clean "$APP";;
    restart) stop "$APP"
        start "$APP";;
    *) usage ;;
esac
