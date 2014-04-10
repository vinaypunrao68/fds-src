#!/bin/bash

LOG_FILE=/tmp/loghelper.log
COLOR_BEGIN="\e[0;"
BOLD_BEGIN="\e[1;"
COLOR_END="\e[0m"
CODE_BLACK=30
CODE_RED=31
CODE_GREEN=32
CODE_YELLOW=33
CODE_BLUE=34
CODE_MAGENTA=35
CODE_CYAN=36
CODE_WHITE=37

function green()      { echo -e "${COLOR_BEGIN}${CODE_GREEN}m$*${COLOR_END}"  ; }
function blue()       { echo -e "${COLOR_BEGIN}${CODE_BLUE}m$*${COLOR_END}"   ; }
function red()        { echo -e "${COLOR_BEGIN}${CODE_RED}m$*${COLOR_END}"    ; }
function yellow()     { echo -e "${COLOR_BEGIN}${CODE_YELLOW}m$*${COLOR_END}" ; }
function white()      { echo -e "${COLOR_BEGIN}${CODE_WHITE}m$*${COLOR_END}"  ; }

function boldgreen()  { echo -e "${BOLD_BEGIN}${CODE_GREEN}m$*${COLOR_END}"   ; }
function boldblue()   { echo -e "${BOLD_BEGIN}${CODE_BLUE}m$*${COLOR_END}"    ; }
function boldred()    { echo -e "${BOLD_BEGIN}${CODE_RED}m$*${COLOR_END}"     ; }
function boldyellow() { echo -e "${BOLD_BEGIN}${CODE_YELLOW}m$*${COLOR_END}"  ; }
function boldwhite()  { echo -e "${BOLD_BEGIN}${CODE_WHITE}m$*${COLOR_END}"   ; }

function log() {  echo "$@" ; }
function logwarn()  { log $(green  "[WARN]  : ") $@ ; }
function logerror() { log $(red    "[ERROR] : ") $@ ; }
function loginfo()  { log $(yellow "[INFO]  : ") $@ ; }
