#! /bin/bash

OPTIND=1         # Reset in case getopts has been used previously in the shell.

no_sudo=0

function show_help {
    echo "Usage: ./prog [-k] hostname"
    echo "Use -k to disable request for sudo password (when passwordless sudo is enabled)"
}

while getopts "h?k" opt; do
    case "$opt" in
    h|\?)
        show_help
        exit 0
        ;;
    k)  no_sudo=1
        ;;
    esac
done

shift $((OPTIND-1))

echo "no_sudo=$no_sudo, Leftovers: $@"

if [ $# -eq 0 ]; then 
    echo "Need to provide a hostname or groupname that is present in the ansible_hosts"
    exit 1
fi

hosts="$1"
script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd -P)"
inventory=${script_dir}/../ansible_hosts
playbooks=${script_dir}/../playbooks

ansible_args=" -i ${inventory} -e "target_hosts=${hosts}" ${playbooks}/deploy_config_files_only.yml"
if [ $no_sudo -eq 0 ]; then 
    echo sudo
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
