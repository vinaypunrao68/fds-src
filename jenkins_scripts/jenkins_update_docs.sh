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
   ./Build/linux-x86_64.debug/bin/thrift --gen html -r source/fdsp/doc_api.thrift
}

function copy_to_webserver
{
   message "UPDATING HOSTED API DOCUMENTS"
   sshpass -p "passwd" scp -r gen-html/ root@coke.formationds.com:/var/www/apis/
}

gen_api_docs
copy_to_webserver
