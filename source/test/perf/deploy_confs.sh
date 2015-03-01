#!/bin/bash

tag=$1
om_node=$2
hosts=`echo $3 | sed 's/,/\\\n/g'`

sed "s/OM_HOST/$om_node/" < regress/config/ansible_hosts.$tag > .inventory.tmp
sed "s/HOST/$hosts/" < .inventory.tmp > ../../../ansible/inventory/$om_node
