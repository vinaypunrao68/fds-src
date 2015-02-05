#!/bin/bash

tag=$1
om_node=$2
hosts=`echo $3 | sed 's/,/\\\n/g'`

sed "s/HOST/$hosts/" < regress/config/ansible_hosts.$tag > ../../../ansible/inventory/$om_node

