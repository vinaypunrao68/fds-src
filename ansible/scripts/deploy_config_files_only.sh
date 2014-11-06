#! /bin/bash

if [ $# -eq 0 ]; then 
    echo "Need to provide a hostname or groupname that is present in the ansible_hosts"
    exit 1
fi

hosts="$1"
script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd -P)"
inventory=${script_dir}/../ansible_hosts
playbooks=${script_dir}/../playbooks
ansible_args=" -i ${inventory} -e "target_hosts=${hosts}" ${playbooks}/deploy_config_files_only.yml"

# Test auth to target hosts

ansible -i ${inventory} ${hosts} -m setup > /dev/null 2>&1

if [ $? -eq 3 ]; then
    echo "Problem connecting as $(whoami) via pub key. Trying again with user-entered credentials..."
    echo -n "SSH username: "
    read ssh_username
    ansible_args="${ansible_args} -u ${ssh_username} -k"
else
    ansible_args="${ansible_args} -K"
fi

ansible-playbook ${ansible_args}

if [ $? -eq 0 ]; then
    echo "SUCCESS - Please review files on target host at /root/fds_configs"
fi
