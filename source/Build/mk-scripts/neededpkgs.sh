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
    libudev-dev libparted-dev
    python-pip
    fds-pkghelper
    fds-pkg
    fds-pkgtools
)

python_packages=(
    paramiko
    redis
    requests
    scp
    pyyaml
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
        oracle-java8.*) 
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
    pkgname=${pkg%%_*}
    if [[ $pkg == *_* ]]; then 
        pkgversion=${pkg##*_}
    else
        pkgversion=""
    fi

    if [[ -z $pkgversion ]] ; then
        pkginfo=$( dpkg-query -f '${Package}' -W $pkgname 2>/dev/null)
    else
        pkginfo=$( dpkg-query -f '${Package}_${Version}' -W $pkgname 2>/dev/null)
    fi
    #echo "${pkginfo} , $pkgname, $pkgversion , $pkg"
    #exit
    if  [[ -z $pkginfo ]] || [[ $pkginfo != $pkg ]] ; then 
        echo "[devsetup] : $pkg is not installed, but needed .. installing."
        preinstall $pkgname
        if [[ -z $pkgversion ]] ; then
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
    name=$(pip freeze 2>/dev/null | grep ^${pkgname}= | cut -f1 -d=)
    if  [[ -z $name ]] ; then 
        echo "[devsetup] : $pkg is not installed, but needed .. installing."
        preinstall $pkg
        sudo pip install $pkg
        postinstall $pkg
    fi
done
