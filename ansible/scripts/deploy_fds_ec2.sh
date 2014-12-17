#! /bin/bash

usage() {
cat << EOF
Usage:
${0##/*} name-tag instance-count local|nightly [debug]
Deploy FDS to a cluster in EC2
name-tag needs to exactly match the name of your inventory file in fds-src/ansible/inventory/
Examples:
${0##/*} cody-ec2 4 nightly
${0##/*} foobar-nodes 4 local
EOF
}

if [ $# -lt 3 ]; then
    usage
    exit 1
fi

if [ "${4}" == "debug" ]; then
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
instance_count="${2}"
deploy_source="${3}"
script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd -P)"
ansible_base_dir="$( cd "${script_dir}/.." && pwd -P)"
playbooks="${ansible_base_dir}/playbooks"
ansible_args="${playbooks}/deploy_fds_ec2.yml -i ${ansible_base_dir}/inventory/${inventory} -e "instance_name_tag=${inventory}" -e "instance_count=${instance_count}" -e "deploy_artifact=${deploy_source}""

D "ansible_args :: ${ansible_args}"

I "Deploying FDS to EC2 with tag: ${inventory}"

ansible-playbook ${ansible_args}
