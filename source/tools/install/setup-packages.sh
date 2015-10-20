#!/bin/bash

###########################################################################
# NOTE:: space/newline separated list of needed pkgs
# the pkgs will be checked / installed in the order as they are 
# mentioned here!!!
# NOTE!!!:  no space between =(  , 
# () means array
# if you wanto specify a specific version , the output of the following 
# command should be specified:
#  ---> dpkg-query -f '${Package}_${Version}' -W $pkgname 
###########################################################################
source ./loghelper.sh

basedebs=(
    libpcre3
    "libconfig++9"
    libev4
    libreadline5
    xfsprogs
    libjemalloc1
    redis-tools
    redis-server
    java-common
    oracle-java8-jdk
    libical1
    libhiredis0.10
    python-setuptools
    python-colorama
    python-distlib
    python-html5lib
    python-pip
)

fdsbasedebs=(
    fds-pkghelper
    fds-pkg
    fds-boost
    fds-leveldb
    fds-python-scp
    fds-jdk-default
)

python_packages=(
    Naked
    crypto
    boto
    ecdsa
    paramiko
    scp
)

###########################################################################
# Fill in the blanks if you need additional actions before installing 
# a specific pkg 
# eg : add a specific ppa.
###########################################################################
function preinstall() {
    local pkgname=$1

    case $pkgname in
        oracle-java8-jdk)
            ;;
        fds-jdk-default)
            ;;
    esac

    # Need to forcibly remove fds-python-paramiko to avoid conflicts
    dpkg -P fds-python-paramiko

    return 0
}

###########################################################################
# actions to be taken after installing a specific package
###########################################################################
function postinstall() {
    local pkgname=$1

    case $pkgname in
        oracle-java8*) 
            ;;
        redis-server*)
            sudo service redis-server stop
            ;;
        fds-jdk-default)
            loginfo "$(red ================================================================)"
            loginfo "$(boldgreen NOTE:: to export the proper java settings into you env...)"
            loginfo "$(boldblue run) : $(boldwhite source /etc/profile.d/jdk.sh)"
            loginfo "$(red ================================================================)"
    esac
}

function installPythonPkgs() {

    loginfo "[fdssetup] : checking python packages...."
    local name
    for pkg in ${python_packages[@]} 
    do 
        loginfo "[fdssetup] : checking python pkg : $pkg"
        pkgname=${pkg}
        #name=$(pip freeze 2>/dev/null | grep ^${pkgname}= | cut -f1 -d=)
        if  [[ -z $name ]] ; then 
            logwarn "[fdssetup] : $pkg is not installed, but needed .. installing."
            preinstall $pkg
            sudo pip install ./${pkg}*
            postinstall $pkg
            echo ""
        fi
    done
}

function installBaseDebs() {
    loginfo "[fdssetup] : installing debs"

    for pkg in ${basedebs[@]}
    do
        loginfo "[fdssetup] : installing $pkg"
        debfile=$(ls -1t "${pkg}"*.deb 2>/dev/null | head -n1)
        if [[ -n $debfile ]] ; then
            sudo dpkg -i "$debfile"
            postinstall "${pkg}"
            echo ""
        else
            logerror "unable to locate : $pkg"
        fi
    done
}
 
function installFdsBaseDebs() {
    loginfo "[fdssetup] : installing fds base debs"

    for pkg in ${fdsbasedebs[@]}
    do
        loginfo "[fdssetup] : installing $pkg"
        debfile=$(ls -1t ${pkg}*.deb 2>/dev/null| head -n1)
        if [[ -n $debfile ]] ; then
            sudo dpkg -i $debfile
            echo ""
        else
            logerror "unable to locate : $pkg"
        fi
    done
}

function installFdsService() {
    local service=$1
    loginfo "[fdssetup] : installing fds service [$service]"
    local pkg
    case $service in
        "fds-platform" ) pkg="fds-platform" ;;
        *)
            logerror "[fdssetup] : unknown fds service [$service]"
            return 1
            ;;
    esac
    
    loginfo "[fdssetup] : installing $pkg"
    debfile=$(ls -1t ${pkg}*.deb 2>/dev/null | head -n1)
    if [[ -n $debfile ]] ; then
        sudo dpkg -i $debfile
        echo ""
    else
        logerror "unable to locate : $pkg"
    fi

    return 0
}



###########################################################################
# ------- MAIN PROGRAM --------
###########################################################################

usage() {
    echo $(yellow "usage: setup-packages.sh option [option ..]")
    echo "option :"
    echo "  base - install base debs"
    echo "  python  - install python pkgs"
    echo "  fds-base - install base fds pkgs"
    echo "  fds-[om/am/pm/dm/sm] - install respective fds service"
    exit 0
}

if [[ $# == 0 ]] ; then
    usage
fi

for opt in "$@" ; do
    case $opt in
        basedeb*) installBaseDebs ;;
        python*) installPythonPkgs ;;
        fds-base*) installFdsBaseDebs ;;
        fds-*) installFdsService $opt ;;
        *) 
            logerror "unknown option $opt" 
            usage
            ;;
    esac
done

