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

needed_packages=(
    python-dev
    python-pip
)

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
)

fdsbasedebs=(
    fds-pkghelper
    fds-pkg
    fds-systemdir
    fds-systemconf
    fds-pythonlibs
    fds-tools
    fds-boost
    fds-leveldb
    fds-python-scp
    fds-python-paramiko
    fds-jdk-default
		libcryptopp-dev
)

python_packages=(
    Crypto
    boto
    scp
    paramiko
)

REPOUPDATED=0
function updateFdsRepo() {
    if [[ $REPOUPDATED == "0" ]]; then
        sudo apt-get update -o Dir::Etc::sourcelist="sources.list.d/fds.list" -o Dir::Etc::sourceparts="-" -o APT::Get::List-Cleanup="0"
        REPOUPDATED=1
    fi
}

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
            sudo easy_install ${pkg}*
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

function installNeededPkgs() {
    loginfo "[fdssetup] : installing prereq packages"
    for pkg in ${needed_packages[@]}
    do
        loginfo "[fdssetup] : installing $pkg"
        sudo apt-get install ${pkg} --assume-yes
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
        "fds-om" ) pkg="fds-orchmgr" ;;
        "fds-am" ) pkg="fds-accessmgr" ;;
        "fds-sm" ) pkg="fds-stormgr" ;;
        "fds-dm" ) pkg="fds-datamgr" ;;
        "fds-pm" ) pkg="fds-platformmgr" ;;
        "fds-cli" ) pkg="fds-cli" ;;
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
    echo "  prereq - install prerequisite packages"
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
        prereq*) installNeededPkgs ;;
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

