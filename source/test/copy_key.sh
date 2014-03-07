#!/bin/bash
USER=`whoami`
KEY_FILE=~/.ssh/authorized_keys
exist=`cat $KEY_FILE | grep "lab-user@fds-lab" | wc -l`
echo $exist
if [ $exist == 0 ]; then
    echo "Copying lab key file to authorized_keys"
    cat test/lab.rsa.pub >> $KEY_FILE
else
    echo "Lab rsa key exist in authorized_keys"
fi
