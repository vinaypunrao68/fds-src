#!/bin/bash

node=$1

sshpass -p passwd ssh -o StrictHostKeyChecking=no -y root@$node ssh-keygen
sshpass -p passwd scp -o StrictHostKeyChecking=no root@$node:.ssh/id_rsa.pub ~/.ssh/id_rsa.pub.$node
cat ~/.ssh/id_rsa.pub.$node >> ~/.ssh/authorized_keys
