#! /bin/bash
usage() {
cat << EOF
Usage:
${0##/*} inventory-file local|nightly [debug]
Deploy SCST to a group of FDS hosts defined in an Ansible inventory file. fds-deps must be deployed first!
inventory-file can be a filename (assumed to exist in {fds-src}/ansible/inventory} or a path to your inventory file
Examples:
${0##/*} bld-nightly-nodes
${0##/*} ../foo/bar/inventories/cody-local-nodes
EOF
}

if [ $# -lt 1 ]; then
    usage
    exit 1
fi

if [ "${2}" == "debug" ]; then
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

PATH=$PATH:/opt/fds-deploy/embedded/bin
inventory="${1}"
script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd -P)"
ansible_base_dir="$( cd "${script_dir}/.." && pwd -P)"
playbooks="${ansible_base_dir}/playbooks"
ansible_args="${playbooks}/deploy_scst.yml"
inventory_filename=$(echo "${inventory}" | awk -F '/' '{ print $NF }')

D "Inventory:           ${inventory}"
D "Ansible Base:        ${ansible_base_dir}"
D "ansible_args         ${ansible_args}"

check_environment() {
    $(egrep '(lxc|docker)' /proc/1/cgroup >/dev/null 2>&1)
    grep_rc=$?

    current_user=$(whoami)

    if [ ${grep_rc} -eq 0 ] && [ "${current_user}" == "root" ]; then
        I "You appear to be running Ansible as root inside a container. For some reason, this will cause fds-tool and fdsadmin calls to fail when run by Ansible."
				I "This may not be a problem any longer so this message is informational & may be removed in the future"
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
    ansible_args="${ansible_args} -i ${inventory}"
}

check_auth() {
    # Test auth to target hosts
    cd ${ansible_base_dir} && ansible-playbook ${ansible_args} --tags check_sudo > /dev/null 2>&1

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
    cd ${ansible_base_dir} && ansible-playbook ${ansible_args} --tags check_sudo > /dev/null 2>&1

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
    # passing -e fds_cluster_name for the sake of getting identifiable metrics
    cd ${ansible_base_dir} && ansible-playbook ${ansible_args} -e "fds_cluster_name=${inventory_filename}" --skip-tags check_sudo --vault-password-file ~/.vault_pass.txt
}

check_environment
check_inventory
check_auth
check_sudo
run_deploy_playbook
