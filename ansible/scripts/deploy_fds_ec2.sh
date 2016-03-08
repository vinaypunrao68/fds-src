#! /bin/bash

usage() {
cat << EOF
Usage:
${0##/*} name-tag instance-count local [debug]
Deploy FDS to a cluster in EC2
name-tag needs to exactly match the name of your inventory file in fds-src/ansible/inventory/
Examples:
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

PATH=$PATH:/opt/fds-deploy/embedded/bin
inventory="${1}"
instance_count="${2}"
deploy_source="${3}"
script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd -P)"
ansible_base_dir="$( cd "${script_dir}/.." && pwd -P)"
playbooks="${ansible_base_dir}/playbooks"

[[ "${deploy_source}" == "nightly" ]] && echo "${0##*/} no longer supports using the nightly package build as an install source.  Please use the nightly installer bundle." && exit 1

# Passing in -e fds_cluster_name for the sake of getting an identifiable set of metrics
ansible_args="${playbooks}/deploy_fds_ec2.yml -i ${ansible_base_dir}/inventory/${inventory} -e "instance_name_tag=${inventory}" -e "instance_count=${instance_count}" -e "deploy_artifact=${deploy_source}" -e "fds_cluster_name=${inventory}""

D "ansible_args :: ${ansible_args}"

I "Deploying FDS to EC2 with tag: ${inventory}"

if [ ${debug} -eq 1 ]; then
    ansible_args="${ansible_args} -vvvv"
fi

cd ${ansible_base_dir} && ansible-playbook ${ansible_args} --vault-password-file ~/.vault_pass.txt

if [ $? -ne 0 ]; then
cat << EOF

*****************************************
*** NOTE: It looks like Ansible failed to provision your nodes in EC2.
***
*** If Ansible is complaining about SSH connection failures, please
***  run this script again with the same args - sometimes it tries to
***  connect prematurely, and I haven't figured out how to fix it yet.
***
*** If, however, it failed on the first task of actually creating one
***  or more instances in EC2, you'll need to MANUALLY TERMINATE the
***  instances that did get created. They remain untagged, and thus
***  Ansible will end up creating more instances than you want (\$\$\$)
EOF
fi
