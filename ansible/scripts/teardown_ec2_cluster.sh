#! /bin/bash

usage() {
cat << EOF
Usage:
${0##/*} name-tag
Tear down an EC2 cluster that matches provided tag
name-tag needs to match the tag that was given to your instances when you ran deploy_fds_ec2.sh
Examples:
${0##/*} cody-ec2
EOF
}

if [ $# -lt 1 ]; then
    usage
    exit 1
fi

name_tag="${1}"
script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd -P)"
ansible_base_dir="$( cd "${script_dir}/.." && pwd -P)"
playbooks="${ansible_base_dir}/playbooks"
ansible_args="${playbooks}/teardown_ec2_cluster.yml -i ${ansible_base_dir}/inventory/localhost -e "instance_name_tag=${name_tag}""

echo "INFO :: Tearing down instances tagged '${name_tag}'"

ansible-playbook ${ansible_args} --vault-password-file ~/.vault_pass.txt
