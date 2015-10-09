#!/bin/bash
USAGE_DESC="operate on ansible playbook directly. command line args will be sent to ansible"
script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd -P)"
source ${script_dir}/script_helpers.sh

parse_args "${@}"
shift

if [[ $# -eq 0 ]]; then 
    I "Eg: --tags activate"
    E "please specific args to pass to ansible"
    exit 0
fi

run_deploy_playbook --tags activate
