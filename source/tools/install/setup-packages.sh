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

basepkgs=(
    oracle-java8-installer oracle-java8-set-default maven

    python-pip
)

basedebs=(
    libjemalloc1_3.6.0.deb
    redis-tools_2.8.8.deb
    redis-server_2.8.8.deb
)

fdsdebs=(
    fds-systemdir_0.1.deb
    fds-systemconf_0.1.deb
    fds-pythonlibs_0.1.deb
    fds-systembin_0.1.deb
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

function installBasePkgs() {
    loginfo "[check-x-depends] : checking ubuntu packages...."

    for pkg in ${basepkgs[@]} 
    do 
        loginfo "[check-x-depends] : checking $pkg ..."
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
    #loginfo "${pkginfo} , $pkgname, $pkgversion , $pkg"
    #exit
        if  [[ -z $pkginfo ]] || [[ $pkginfo != $pkg ]] ; then 
            loginfo "[check-x-depends] : $pkg is not installed, but needed .. installing."
            preinstall $pkgname
            if [[ -z $pkgversion ]] ; then
                sudo apt-get install ${pkgname}
            else
                sudo apt-get install ${pkgname}=${pkgversion}
            fi
            postinstall $pkgname
        fi
    done
}

function installBaseDebs() {
    loginfo "[check-x-depends] : installing debs"

    for pkg in ${basedebs[@]}
    do
        loginfo "[check-x-depends] : installing $pkg"
        if [[ -e $pkg ]] ; then
            sudo dpkg -i $pkg
        else
            logerror "unable to locate : $pkg"
        fi
    done

}

function installPythonPkgs() {

    loginfo "[check-x-depends] : checking python packages...."

    for pkg in ${python_packages[@]} 
    do 
        loginfo "[check-x-depends] : checking python pkg : $pkg"
        pkgname=${pkg}
        name=$(pip freeze 2>/dev/null | grep ^${pkgname}= | cut -f1 -d=)
        if  [[ -z $name ]] ; then 
            logwarn "[check-x-depends] : $pkg is not installed, but needed .. installing."
            preinstall $pkg
            sudo pip install $pkg
            postinstall $pkg
        fi
    done
}

function installFdsDebs() {
    loginfo "[check-x-depends] : installing fds debs"

    for pkg in ${fdsdebs[@]}
    do
        loginfo "[check-x-depends] : installing $pkg"
        if [[ -e $pkg ]] ; then
            sudo dpkg -i $pkg
        else
            logerror "unable to locate : $pkg"
        fi
    done
}

###########################################################################
# ------- MAIN PROGRAM --------
###########################################################################

for opt in "$@" ; do
    case $opt in 
        basepkgs) installBasePkgs ;;
        basedebs) installBaseDebs ;;
        python*) installPythonPkgs ;;
        fdsdebs) installFdsDebs ;;
        *) 
            logerror "unknown option $opt"
            ;;
    esac
done

