#!/bin/bash

if [[ $EUID != 0 ]] ; then
    echo "You need root permissions to execute this script. sudo ?"
    exit 1
fi


REPOFILE=/etc/apt/sources.list.d/fds.list

echo "setting up fds repository...."

echo "
######################################################
# fds repo
######################################################

deb http://coke.formationds.com:8000 fds test" > $REPOFILE

echo "adding fds gpg key to the system"

wget -O - http://coke.formationds.com:8000/fds.repo.gpg.key 2>/dev/null | apt-key add -
