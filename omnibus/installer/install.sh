#!/bin/bash

# Include fds-deps in our path
export PATH="$PATH:/opt/fds-deps/bin:/opt/fds-deps/embedded/bin"
MYDIR="$( dirname "${BASH_SOURCE[0]}" )"

while getopts "i:" opt; do
  case $opt in
    i)
      inventory=$OPTARG
      ;;
  esac
done

if [[ -z $inventory ]] ; then
  echo "Please specify the full path to your inventory with -i <inventory>"
  exit 1
fi

function check_root() {
  if [[ $EUID -ne 0 ]]; then
    echo "This option must be run as root"
    exit 1
  fi
}

function check_vault_pass() {
  if [ -f ~/.vault_pass.txt ] ; then
    return
  else
    echo -n "No vault pass file found, please enter the vault pass (will not echo): "
    read -s vault_pass
    echo $vault_pass >> ~/.vault_pass.txt
  fi
}

function install_deps() {
  check_root
  # Make sure we are in our install dir
  cd $MYDIR
  dpkg -i omnibus/omnibus-fds-deps/pkg/*.deb

}


function run_deploy() {

  echo "Running ansible"
  cd $MYDIR/ansible/scripts
  ./deploy_fds.sh $inventory local
}
check_vault_pass
install_deps
run_deploy
