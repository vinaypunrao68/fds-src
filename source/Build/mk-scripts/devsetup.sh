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

needed_packages=(
    redis-server
    oracle-java8-installer oracle-java8-set-default maven
    libudev-dev 
    libparted0-dev
    python2.7
    python-dev
    python-pip

    libpcre3-dev
    libpcre3
    libssl-dev
    libfuse-dev

    libevent-dev

    bison
    flex
    ragel
    ccache

    libboost-log1.55-dev
    libboost-program-options1.55-dev
    libboost-timer1.55-dev
    libboost-thread1.55-dev
    libboost-regex1.55-dev

    fds-pkghelper
    fds-pkg
    fds-pkgtools

    fds-leveldb
    fds-leveldb-dev

    fds-systemdir
    fds-coredump
)

python_packages=(
    paramiko
    redis
    requests
    scp
    PyYAML
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
        oracle-java8*) 
            sudo add-apt-repository ppa:webupd8team/java
            sudo apt-get update
            ;;
        redis-server)
            sudo add-apt-repository ppa:chris-lea/redis-server
            ;;
        fds*)
            updateFdsRepo
            ;;
    esac
}

###########################################################################
# actions to be taken after installing a specific package
###########################################################################
function postinstall() {
    local pkgname=$1

    case $pkgname in
        oracle-java8.*) 
            ;;
        redis-server)
            ;;
    esac
}

###########################################################################
# ------- MAIN PROGRAM --------
###########################################################################

echo "[devsetup] : checking ubuntu packages...."

for pkg in ${needed_packages[@]} 
do 
    echo "[devsetup] : checking $pkg ..."
    pkgname=${pkg%%=*}
    if [[ $pkg == *=* ]]; then 
        pkgversion=${pkg##*=}
    else
        pkgversion=""
    fi

    if [[ -z $pkgversion ]] ; then
        pkginfo=$( dpkg-query -f '${Package}' -W $pkgname 2>/dev/null)
    else
        pkginfo=$( dpkg-query -f '${Package}=${Version}' -W $pkgname 2>/dev/null)
    fi

    # check the pkg state
    if [[ -n $pkginfo ]] ; then
        pkgstate=$(dpkg-query -f '${db:Status-Abbrev}' -W $pkgname 2>/dev/null)
        if [[ $pkgstate != "ii " ]]; then 
            echo "[devsetup] : $pkgname is in unknown state [$pkgstate] - forcing install"
            pkginfo=
        fi
    fi

    #echo "${pkginfo} , $pkgname, $pkgversion , $pkg"
    #exit
    if  [[ -z $pkginfo ]] || [[ $pkginfo != $pkg ]] ; then 
        if [[ $pkgversion == "latest" ]] ; then
            echo "[devsetup] : trying to install the latest version of $pkgname ..."
        else
            echo "[devsetup] : $pkg is not installed, but needed .. installing."
        fi
        preinstall $pkgname
        if [[ -z $pkgversion ]] || [[ $pkgversion == "latest" ]] ; then
            sudo apt-get install ${pkgname}
        else
            sudo apt-get install ${pkgname}=${pkgversion}
        fi
        postinstall $pkgname
    fi
done

echo "[devsetup] : checking python packages...."

for pkg in ${python_packages[@]} 
do 
    pkgname=${pkg}
    echo "[devsetup] : [python] checking : $pkg"
    name=$(pip freeze 2>/dev/null | grep ^${pkgname}= | cut -f1 -d=)
    if  [[ -z $name ]] ; then 
        echo "[devsetup] : $pkg is not installed, but needed .. installing."
        preinstall $pkg
        sudo pip install $pkg
        postinstall $pkg
    fi
done
