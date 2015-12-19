#! /bin/bash
#
# Provisioning EC2 clusters with static IP addresses
#
# This script and the associated playbook are design to deploy a cluster
# given an inventory file and a vars file which provides a list of static
# IP addresses to use. This provides a predictable way to test large domains
# vs. the dynamic cluster scripts. 
#
# If you create a new inventory file you MUST make sure it uses unique IP
# addresses or you and probably someone else are gonna have a bad time.

# Provide some help
function usage() {
cat << EOF
Usage:
${0##/*} [Options] name-tag
Options:
  -h -- This help
  -v -- Enable verbosity in ansible
  -t -- Build type local|nightly|beta [ default: "nightly" ]
  -a -- Action (deploy | stop | start | destroy | clean | cleanstart) [ default: deploy ]
Deploy FDS to a cluster in EC2
name-tag needs to exactly match the name of your inventory file in fds-src/ansible/inventory/
Actions:
  - deploy      : Provision instances & deploy FDS to them
  - stop        : Stop all instances matching name-tag
  - start       : Start all instances matching name-tag
  - destroy     : Destroy all instances matching name-tag
  - clean       : Only clean the domain and don't start FDS
  - cleanstart  : Clean domain & Restart FDS
Examples:
${0##/*} -t nightly kung-ec2
${0##/*} -t local foobar-nodes
${0##/*} -a stop foobar-nodes
${0##/*} -va destroy foobar-nodes
EOF
}

OPTIND=1

# Defaults
verbose=0             # -v option off
build_type="nightly"  # -t nightly
action="deploy"       # -a deploy

while getopts "h?va:t:" opt; do
  case "$opt" in
    h|\?)
      usage
      exit 0
      ;;
    v) verbose=1
      ;;
    t) build_type=$OPTARG
      ;;
    a) action=$OPTARG
      ;;
  esac
done

shift $((OPTIND-1))

# We require that an inventory name is passed in
if [ $# -lt 1 ]; then
    usage
    exit 1
fi

# Provide verbose output
function D() {
    if [ ${verbose} -eq 1 ]; then
        echo -e "verbose :: ${1}\n"
    fi
}

# Info level output
function I() {
    echo -e "INFO :: ${1}"
}

# Warning output
function W() {
    echo -e "WARN :: ${1}"
}

# Error output
function E() {
    echo -e "ERROR :: ${1}"
    exit 1
}

# Confirmation dialogue as long as you are OK with an exit
function confirm() {
  echo -n ${1}
  read user_response
  if [[ "${user_response}" != "yes" ]] ; then
    echo "Exiting then..."
    exit 1
  fi
}

PATH=$PATH:/opt/fds-deploy/embedded/bin
inventory="${1}"
deploy_source="${build_type}"
script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd -P)"
ansible_base_dir="$( cd "${script_dir}/.." && pwd -P)"
playbooks="${ansible_base_dir}/playbooks"

# Get aws credentials for the dynamic inventory script
export AWS_ACCESS_KEY_ID=$(ansible-vault view ${ansible_base_dir}/vars_files/credentials/aws_fds_testing.yml --vault-password-file=~/.vault_pass.txt | grep aws_access_key | awk '{ print $2 }')
export AWS_SECRET_ACCESS_KEY=$(ansible-vault view ${ansible_base_dir}/vars_files/credentials/aws_fds_testing.yml --vault-password-file=~/.vault_pass.txt | grep aws_secret_key | awk '{ print $2 }')

# For each action we setup the appropriate ansible args
case "$action" in
  deploy)
    D "Deploying..."
    ansible_args="${playbooks}/provision_fds_static_ec2.yml -i ${ansible_base_dir}/inventory/${inventory}"
    ;;
  clean)
    D "Cleaning..."
    ansible_args="${playbooks}/provision_fds_static_ec2.yml -i ${ansible_base_dir}/inventory/${inventory} \
      -e clean_fds=yes -e force=yes -e fds_only_install=yes\
      --skip-tags uninstall,install-deps,install-local,install-repo,post-install"
    ;;
  cleanstart)
    D "Cleaning and starting the domain..."
    ansible_args="${playbooks}/provision_fds_static_ec2.yml -i ${ansible_base_dir}/inventory/${inventory} \
      -e clean_fds=yes -e force=yes -e fds_only_install=no\
      --skip-tags uninstall,install-deps,install-local,install-repo,post-install"
    ;;
  stop)
    D "Stopping..."
    ansible_args="${playbooks}/provision_fds_static_ec2.yml -i ${ansible_base_dir}/ec2.py -e instance_state='stopped'"
    ;;
  start)
    D "Starting..."
    ansible_args="${playbooks}/provision_fds_static_ec2.yml -i ${ansible_base_dir}/ec2.py -e instance_state='running'"
    ;;
  destroy)
    D "Terminating..."
    confirm "This will destroy all instances in this cluster - are you sure? (type 'yes'): "
    ansible_args="${playbooks}/provision_fds_static_ec2.yml -i ${ansible_base_dir}/ec2.py -e instance_state='absent'"
    ;;
  *)
    echo "Unknown action: \"${action}\" - Exiting"
    exit 1
    ;;
esac

D "ansible_args :: ${ansible_args}"

# When verbose is enabled we enable debug in ansible as well
if [ ${verbose} -eq 1 ]; then
    ansible_args="${ansible_args} -vvvv"
fi

# Run ansible
cd ${ansible_base_dir} && ansible-playbook ${ansible_args} -e instance_name_tag=${inventory} -e action=${action} -e deploy_artifact=${deploy_source} -e fds_cluster_name=${inventory} --vault-password-file ~/.vault_pass.txt

if [ $? -ne 0 ]; then
cat << EOF

*****************************************
*** NOTE: It looks like Ansible failed to stop your nodes in EC2.
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
