#!/bin/bash

LOG_FILE=/tmp/loghelper.${UID}.log
COLOR_BEGIN="\e[0;"
BOLD_BEGIN="\e[1;"
COLOR_END="\e[0m"
CODE_BLACK=30m
CODE_RED=31m
CODE_GREEN=32m
CODE_YELLOW=33m
CODE_BLUE=34m
CODE_MAGENTA=35m
CODE_CYAN=36m
CODE_WHITE=37m

# disable colors on a non terminal output
if [[ ! -t 1 ]]; then
    COLOR_BEGIN=
    BOLD_BEGIN=
    COLOR_END=
    CODE_BLACK=
    CODE_RED=
    CODE_GREEN=
    CODE_YELLOW=
    CODE_BLUE=
    CODE_MAGENTA=
    CODE_CYAN=
    CODE_WHITE=
fi

function green()      { echo -e "${COLOR_BEGIN}${CODE_GREEN}$*${COLOR_END}"  ; }
function blue()       { echo -e "${COLOR_BEGIN}${CODE_BLUE}$*${COLOR_END}"   ; }
function red()        { echo -e "${COLOR_BEGIN}${CODE_RED}$*${COLOR_END}"    ; }
function yellow()     { echo -e "${COLOR_BEGIN}${CODE_YELLOW}$*${COLOR_END}" ; }
function white()      { echo -e "${COLOR_BEGIN}${CODE_WHITE}$*${COLOR_END}"  ; }

function boldgreen()  { echo -e "${BOLD_BEGIN}${CODE_GREEN}$*${COLOR_END}"   ; }
function boldblue()   { echo -e "${BOLD_BEGIN}${CODE_BLUE}$*${COLOR_END}"    ; }
function boldred()    { echo -e "${BOLD_BEGIN}${CODE_RED}$*${COLOR_END}"     ; }
function boldyellow() { echo -e "${BOLD_BEGIN}${CODE_YELLOW}$*${COLOR_END}"  ; }
function boldwhite()  { echo -e "${BOLD_BEGIN}${CODE_WHITE}$*${COLOR_END}"   ; }

function log() {
    echo "$@"
    echo "$@" >> $LOG_FILE
}
function logwarn()  { log $(green  "[WARN]  : ") $@ ; }
function logerror() { log $(red    "[ERROR] : ") $@ ; }
function loginfo()  { log $(yellow "[INFO]  : ") $@ ; }

function init_loghelper() {
    if [ $# -eq 1 ]; then
        rm -f $1 2>/dev/null
        LOG_FILE=$1
    fi
}
