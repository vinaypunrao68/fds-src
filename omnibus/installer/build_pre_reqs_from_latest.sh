#!/bin/bash

if [ "${1}" != "release" ] && [ "${1}" != "debug" ]
then
    echo "You must supply an argument of either 'release' or 'debug' to indicate what type of build you would like to perform"
    exit 1;
fi

#first make it so the path we're in is fds-src/omnibus/installer

location="${PWD##*/}"
fdssrc="fds-src"
omni="omnibus/installer"

if [[ ${location} == ${fdssrc} ]]
then
    echo "Switching to the omnibus installer directory"
    cd omnibus/installer
fi

if [[ ! -e "build_install.sh" ]]
then
    echo "Please run this script from fds-src or ${omni}"
    exit 1
fi

#This script is to setup the pre-requisites for creating an offline
#deployment.  The only arg is which build type needs to be created.
cd ../.. 
make package fds-platform BUILD_TYPE=${1}
make package fds-deps
cd omnibus/installer

# making directories that the build_install script will look for packages

mkdir -p ../omnibus-fds-stats-service/pkg
mkdir -p ../omnibus-fds-stats-deps/pkg
mkdir -p ../omnibus-rabbitmq-c/pkg
mkdir -p ../omnibus-simpleamqpclient/pkg
mkdir -p ../omnibus-fds-stats-client-c/pkg

apt-get download fds-stats-service-1.0.0
[[ $? -ne 0 ]] && echo 'Failure downloading the fds-stats-service from apt repo' && exit 99
mv fds-stats-service*.deb ../omnibus-fds-stats-service/pkg

apt-get download fds-stats-deps
[[ $? -ne 0 ]] && echo 'Failure downloading the fds-stats-deps from apt repo' && exit 99
mv fds-stats-deps*.deb ../omnibus-fds-stats-deps/pkg

apt-get download rabbitmq-c-0.7.1
[[ $? -ne 0 ]] && echo 'Failure downloading the rabbitmq-c package from apt repo' && exit 99
mv rabbitmq-c*.deb ../omnibus-rabbitmq-c/pkg

apt-get download simpleamqpclient-0.7.1
[[ $? -ne 0 ]] && echo 'Failure downloading the simpleamqpclient package from apt repo' && exit 99
mv simpleamqpclient*.deb ../omnibus-simpleamqpclient/pkg

apt-get download fds-stats-client-c-1.0.0
[[ $? -ne 0 ]] && echo 'Failure downloading the fds-stats-client-c package from apt repo' && exit 99
mv fds-stats-client-c*.deb ../omnibus-fds-stats-client-c/pkg

if [[ ${#JENKINS_URL} -gt 0 ]]
then
    cd ../../jenkins_scripts
    echo "Uploading the platform and fds-deps packages to artifactory"
    ./deploy_artifacts.sh
    cd ../omnibus/installer
fi

echo "Building the offline installer."
if [[ ${#JENKINS_URL} -gt 0 ]]
then
    ./build_install.sh -u
else
    ./build_install.sh
fi
