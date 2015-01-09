#! /bin/bash

OPTIND=1         # Reset in case getopts has been used previously in the shell.

no_sudo=0

function show_help {
    echo "Usage: ./prog [-s] hostname"
    echo "Use -s to disable request for sudo password (when passwordless sudo is enabled)"
}

while getopts "h?s" opt; do
    case "$opt" in
    h|\?)
        show_help
        exit 0
        ;;
    s)  no_sudo=1
        ;;
    esac
done

shift $((OPTIND-1))

echo "no_sudo=$no_sudo, Leftovers: $@"

if [ $# -eq 0 ]; then 
    echo "Need to provide a valid inventory name" 
    exit 1
fi

script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd -P)"
inventory=${script_dir}/../inventory/${1}
playbooks=${script_dir}/../playbooks

echo "INVENTORY: ${inventory}"

ansible_args=" -i ${inventory} ${playbooks}/deploy_config_files_only.yml"
if [ $no_sudo -eq 0 ]; then 
    ansible_args="$ansible_args -K"
fi

# Test auth to target hosts
ansible -i ${inventory} ${hosts} -m ping > /dev/null 2>&1

if [ $? -eq 3 ]; then
    echo "Problem connecting as $(whoami) via pub key. Trying again with user-entered credentials..."
    echo -n "SSH username: "
    read ssh_username
    ansible_args="${ansible_args} -u ${ssh_username} -k"
fi

ansible-playbook ${ansible_args}

if [ $? -eq 0 ]; then
    echo "SUCCESS - Please review files on target host at /root/fds_configs"
fi
