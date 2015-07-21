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
        rmdir ${sem_dir}
    fi
fi

mkdir "${sem_dir}" 2>/dev/null

case $? in
    0)
        if [ ! -f /usr/local/bin/ansible-playbook ]; then
            sudo apt-get install -y --force-yes python-pip python-dev
            sudo -H pip install "ansible==1.8.4"
        fi

        rm -f ${script_dir}/.devsetup-is-up-to-date

        # Construct the collection of ansible args

        ansible_args="--inventory ${script_dir}/inventory/devsetup-inventory.sh --connection local 
                      ${script_dir}/playbooks/devsetup.yml"

        # FIXME: Does this REALLY need to be world-writable?
        [[ -d ~/.ansible ]] && sudo chmod -R a+w ~/.ansible
        sudo ansible-playbook ${ansible_args}

        if [ $? -eq 0 ]; then
            touch ${script_dir}/.devsetup-is-up-to-date
        else
            rmdir ${sem_dir}
            exit 1
        fi

        rmdir ${sem_dir}
        ;;
    *)
    while [[ -d ${sem_dir} ]]
    do
        sleep 1
    done
    ;;
esac
