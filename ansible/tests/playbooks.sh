#!/usr/bin/env bash
set -o errexit
set -o nounset
# set -o xtrace
set -o pipefail

script_dir="$(cd "$(dirname "${0}")"; echo $(pwd))"
playbook_dir="$(cd "${script_dir}/../playbooks"; echo $(pwd))"
script_base="$(basename "${0}")"
script_full="${script_dir}/${script_base}"

echo "################################"
echo "Build Information"
echo "Directory: ${script_dir}"
echo "Filename: ${script_full}"
echo "Playbook Directory: ${playbook_dir}"
echo "Version Information:"
echo "Ansible Version: $(ansible --version)"
echo "Ansible Playbook Version: $(ansible-playbook --version)"
echo "Operating System: $(lsb_release -d | awk -F: '{ print $2 }' | tr -d '\t')"
echo "Kernel: $(uname -a)"
echo "################################"

echo "### Starting tests"

find ${playbook_dir} -maxdepth 1 -name '*.yml'| xargs -n1  ansible-playbook --syntax-check --list-tasks -i localhost
