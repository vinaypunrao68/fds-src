#! /bin/bash 
usage() {
cat << EOF
Usage:
${0##/*} inventory-file local|nightly [debug]
Deploy FDS to a group of hosts defined in an Ansible inventory file. Can deploy from local Debs or Artifactory release channels.
inventory-file can be a filename (assumed to exist in {fds-src}/ansible/inventory} or a path to your inventory file
Examples:
${0##/*} bld-nightly-nodes nightly
${0##/*} ../foo/bar/inventories/cody-local-nodes local
EOF
}

if [ $# -lt 2 ]; then 
    usage
    exit 1
fi

if [ "${3}" == "debug" ]; then
    debug=1
else
    debug=0
fi

D() {
    if [ ${debug} -eq 1 ]; then
        echo -e "DEBUG :: ${1}\n"
    fi
}

I() {
    echo -e "INFO :: ${1}"
}

W() {
    echo -e "WARN :: ${1}"
}

E() {
    echo -e "ERROR :: ${1}"
    exit 1
}

inventory="${1}"
deploy_source="$2"
script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd -P)"
ansible_base_dir="$( cd "${script_dir}/.." && pwd -P)"
playbooks="${ansible_base_dir}/playbooks"
ansible_args="${playbooks}/deploy_fds.yml"

D "Inventory:           ${inventory}"
D "Deploy Artifact:     ${deploy_source}"
D "Ansible Base:        ${ansible_base_dir}"
D "ansible_args         ${ansible_args}"

check_environment() {
    $(egrep '(lxc|docker)' /proc/1/cgroup >/dev/null 2>&1)
    grep_rc=$?

    current_user=$(whoami)

    if [ ${grep_rc} -eq 0 ] && [ "${current_user}" == "root" ]; then
        E "ERROR: You appear to be running Ansible as root inside a container. For some reason, this will cause fds-tool and fdsadmin calls to fail when run by Ansible.\n\nPlease run this as a non-root user within your container, or as root from a non-container.\n"
        exit 1
    fi
}

check_inventory() {
    # Check that the inventory specified actually exists

    if [[ ! ${inventory} =~ / ]]; then
        inventory=${ansible_base_dir}/inventory/${inventory}
    fi    

    [ -f ${inventory} ] || inventory_is_missing=1

    if [ ${inventory_is_missing} ]; then
        E "Inventory file ${inventory} does not exist."
    fi

    D "Inventory File ${inventory} exists, continuing" 
    ansible_args="${ansible_args} -i $inventory"
}

check_auth() {
    # Test auth to target hosts
    ansible-playbook ${ansible_args} --tags check_sudo > /dev/null 2>&1

    if [ $? -eq 3 ]; then
        W "Problem connecting with inventory-defined credentials. Please provide credentials I can use to get your nodes deployment-ready:"
        echo -n "SSH username: "
        read ssh_username
        echo -n "SSH password: "
        read -s ssh_password
        echo

        ansible_args="${ansible_args} -e "ansible_ssh_user=${ssh_username}" -e "ansible_ssh_pass=${ssh_password}""
    else
        D "Inventory credentials are good, continuing"
    fi

    D "ansible_args         ${ansible_args}"
}

check_sudo() {
    # Check if user has passwordless sudo privileges
    ansible-playbook ${ansible_args} --tags check_sudo > /dev/null 2>&1

    if [ $? -eq 2 ]; then
        echo
        W "It looks like user '${ssh_username}' doesn't have password-less sudo privileges on all hosts. Please provide the sudo password:"
        echo -n "sudo password: "
        read -s sudo_password
        echo
        echo

        ansible_args="${ansible_args} -e "ansible_sudo_pass=${sudo_password}""
    else
        D "passwordless sudo is setup, continuing"
    fi

    D "ansible_args         ${ansible_args}"
}

run_deploy_playbook() {
    for hostname in $(ansible all --list-hosts -i inventory/bld-nightly-nodes) ; 
        do ssh-keygen -R ${hostname}
    done

    ansible-playbook ${ansible_args} -e "deploy_artifact=${deploy_source}" --skip-tags check_sudo
}

check_environment
check_inventory
check_auth
check_sudo
run_deploy_playbook
