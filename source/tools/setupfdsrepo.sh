#!/bin/bash

# check if this is a super user
if [[ $EUID != 0 ]] ; then
    echo "[reposetup] : You need root permissions to execute this script. sudo ?"
    exit 
fi

# check if the user can resolve the name
if [[ -z $(dig +short coke.formationds.com) ]]; then
    echo "[reposetup] : unable to resolve coke.formationds.com .. Are you inside the fds network ??"
    exit 100
fi

REPOFILE=/etc/apt/sources.list.d/fds.list

echo "[reposetup] : setting up fds repository...."

echo "
######################################################
# fds repo
######################################################

deb http://coke.formationds.com:8000 fds test

" > $REPOFILE

echo "[reposetup] : adding fds gpg key to the system ..."

wget -O - http://coke.formationds.com:8000/fds.repo.gpg.key 2>/dev/null | apt-key add -
