#!/usr/bin/env bash
#grep "$ANSIBLE_VAULT;" filename
set -o nounset
# set -o xtrace
set -o pipefail

red='\033[0;31m'
green='\033[0;32m'
nocolor='\033[0m'

script_dir="$(cd "$(dirname "${0}")"; echo $(pwd))"
playbook_dir="$(cd "${script_dir}/../playbooks"; echo $(pwd))"
creds_dir="$(cd "${script_dir}/../vars_files/credentials"; echo $(pwd))"
script_base="$(basename "${0}")"
script_full="${script_dir}/${script_base}"
exit_code=0

echo "################################"
echo "Build Information"
echo "Directory: ${script_dir}"
echo "Filename: ${script_full}"
echo "Playbook Directory: ${playbook_dir}"
echo "Credentials Directory: ${creds_dir}"
echo "Version Information:"
echo "Ansible Version: $(ansible --version)"
echo "Ansible Playbook Version: $(ansible-playbook --version)"
echo "Operating System: $(lsb_release -d | awk -F: '{ print $2 }' | tr -d '\t')"
echo "Kernel: $(uname -a)"
echo "################################"
echo

get_vault_pw_file() {
    echo "Retrieving vault credentials..."
    wget_results=$(wget -O .vault_pass.txt http://bld-dev-02/cda.dat >/dev/null 2>&1)

    if [ $? -ne 0 ]; then
        echo "FAILED to retrieve vault credentials, bailing out"
        cleanup_and_exit 2
    fi

    echo "Succesfully retrieved vault credentials "
    echo
    echo "### Starting tests"
    echo
}

test_cleartext_credentials() {
    failing_files=()

    echo "-------------------------------"
    echo "CLEARTEXT CREDENTIAL TEST START"
    echo "-------------------------------"

    for filename in $(find ${creds_dir} -maxdepth 1 -name '*.yml'); do
        echo
        grep "\$ANSIBLE_VAULT" "${filename}" > /dev/null 2>&1

        if [ $? -ne 0 ]; then
            echo -e "${red}(FAIL) :: ${filename}${nocolor}"
            failing_files+=(${filename})
        else
            echo -e "${green}(PASS) :: ${filename}${nocolor}"
        fi
    done

    echo
    echo "--------------------------------"
    echo "CLEARTEXT CREDENTIAL TEST RESULT"
    echo "--------------------------------"

    if [ ${#failing_files[@]} -gt 0 ]; then
        echo -e "${red}(FAIL) :: Found unencrypted credentials files"
        echo "IE has been notified. Credentials will be revoked from associated services."
        echo -e "Things are going to break, please contact IE ASAP.${nocolor}"
        exit_code=1
    else
        echo -e "${green}(PASS) :: No unencrypted credentials files found${nocolor}"
    fi
}

test_playbook_sanity() {
    failing_files=()

    echo "--------------------------"
    echo "PLAYBOOK SANITY TEST START"
    echo "--------------------------"

    for filename in $(find ${playbook_dir} -maxdepth 1 -name '*.yml'); do
        echo
        ansible_results=$(ansible-playbook -i ${script_dir}/localhost --syntax-check --list-tasks --vault-password-file ./.vault_pass.txt "${filename}" 2>&1)

        if [ $? -ne 0 ]; then
            echo -e "${red}(FAIL) :: ${filename}"
            echo -e "${ansible_results}${nocolor}"
            failing_files+=(${filename})
        else
            echo -e "${green}(PASS) :: ${filename}${nocolor}"
        fi
    done

    echo
    echo "---------------------------"
    echo "PLAYBOOK SANITY TEST RESULT"
    echo "---------------------------"

    if [ ${#failing_files[@]} -gt 0 ]; then
        echo -e "${red}(FAIL) :: Found errors in one or more playbooks${nocolor}"
        exit_code=1
    else
        echo -e "${green}(PASS) :: No playbook errors found${nocolor}"
    fi
}

cleanup_and_exit() {
    echo
    echo "Cleaning up and exiting with return code ${1}"
    echo
    rm -f ./.vault_pass.txt
    exit ${1}
}

get_vault_pw_file
test_cleartext_credentials
test_playbook_sanity

cleanup_and_exit ${exit_code}
