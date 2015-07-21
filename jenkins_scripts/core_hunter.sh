#!/bin/bash -l

. ./jenkins_scripts/message.sh

function core_hunter
{
    message  "POKING around for core files"
    find /corefiles -type f -name "*.core" |grep -e ".*" > /dev/null
    return_code=$?
    return ${return_code}
}

