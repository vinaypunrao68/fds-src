#!/bin/bash
USAGE_DESC="Activate the base local fds domain"
script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd -P)"
source ${script_dir}/script_helpers.sh

parse_args "${@}"
deploy_source=""
run_deploy_playbook "${@}"
