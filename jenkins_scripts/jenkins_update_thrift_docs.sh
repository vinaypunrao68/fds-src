#!/bin/bash -le

function message
{
   echo "================================================================================"
   echo "$*"
   echo "================================================================================"
}

function gen_api_docs
{
   message "GENERATE API THRIFT DOCUMENTS"
   Build/linux-x86_64.debug/bin/thrift --gen html -r source/fdsp/doc_api.thrift
}

function copy_to_webserver
{
   message "UPDATING HOSTED API DOCUMENTS"
   sshpass -p "passwd" scp -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no -r gen-html/ root@coke.formationds.com:/var/www/apis/
}

[[ "${BUILD_ON_COMMIT}"x == "true"x ]] || exit 0

gen_api_docs

# ensure we are on a Jenkins builder
[[ ${#JENKINS_URL} -gt 0 ]] && copy_to_webserver
