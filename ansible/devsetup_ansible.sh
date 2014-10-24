#!/bin/bash

### README ###
#   This is only a helper script to quell parallel executions of the devsetup Ansible playbook
#   If you need to make changes to devsetup, this is NOT LIKELY to be the file you need to edit

script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd -P)"
sem_dir="${script_dir}/${0##*/}.semaphore"
sem_expiration_seconds=1800

if [ -d ${sem_dir} ]; then
	dir_mtime=$(stat -c %Y ${sem_dir})
	current_time=$(date +"%s")

	if [ $((current_time - dir_mtime)) -gt ${sem_expiration_seconds} ]; then
		echo "old semaphore expired - removing"
		rmdir ${sem_dir}
	fi
fi

mkdir "${sem_dir}" 2>/dev/null

case $? in
	0)
		echo "running devsetup"
		ansible-playbook -i ${script_dir}/ansible_hosts -c local ${script_dir}/playbooks/devsetup.yml
		rmdir ${sem_dir}
		touch ${script_dir}/.devsetup-is-up-to-date
		;;
	*)
	while [[ -d ${sem_dir} ]]
	do
		sleep 1
	done
	echo "devsetup already running, slept until complete - continuing now"
	;;
esac
