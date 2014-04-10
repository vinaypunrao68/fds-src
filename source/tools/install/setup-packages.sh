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
    libpcre3
)

basedebs=(
    libjemalloc1_*.deb
    redis-tools_*.deb
    redis-server_*.deb
)

fdsbasedebs=(
    fds-pkghelper_*.deb
    fds-systemdir_*.deb
    fds-systemconf_*.deb
    fds-pythonlibs_*.deb
    fds-boost*.deb
    fds-leveldb*.deb
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
        redis-server*)
            sudo service redis-server stop
            ;;
    esac
}

function installBasePkgs() {
    loginfo "[fdssetup] : checking ubuntu packages...."

    for pkg in ${basepkgs[@]} 
    do 
        loginfo "[fdssetup] : checking $pkg ..."
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
        #  exit
        if  [[ -z $pkginfo ]] || [[ $pkginfo != $pkg ]] ; then 
            loginfo "[fdssetup] : $pkg is not installed, but needed .. installing."
            preinstall $pkgname
            if [[ -z $pkgversion ]] ; then
                sudo apt-get install ${pkgname}
            else
                sudo apt-get install ${pkgname}=${pkgversion}
            fi
            postinstall $pkgname
            echo ""
        fi
    done
}

function installBaseDebs() {
    loginfo "[fdssetup] : installing debs"

    for pkg in ${basedebs[@]}
    do
        loginfo "[fdssetup] : installing $pkg"
        if [[ -e $pkg ]] ; then
            sudo dpkg -i $pkg
            postinstall $pkg
            echo ""
        else
            logerror "unable to locate : $pkg"
        fi
    done

}

function installPythonPkgs() {

    loginfo "[fdssetup] : checking python packages...."

    for pkg in ${python_packages[@]} 
    do 
        loginfo "[fdssetup] : checking python pkg : $pkg"
        pkgname=${pkg}
        name=$(pip freeze 2>/dev/null | grep ^${pkgname}= | cut -f1 -d=)
        if  [[ -z $name ]] ; then 
            logwarn "[fdssetup] : $pkg is not installed, but needed .. installing."
            preinstall $pkg
            sudo pip install $pkg
            postinstall $pkg
            echo ""
        fi
    done
}

function installFdsBaseDebs() {
    loginfo "[fdssetup] : installing fds base debs"

    for pkg in ${fdsbasedebs[@]}
    do
        loginfo "[fdssetup] : installing $pkg"
        if [[ -e $pkg ]] ; then
            sudo dpkg -i $pkg
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
        "fds-om" ) pkg="fds-orchmgr*.deb" ;;
        "fds-am" ) pkg="fds-accessmgr*.deb" ;;
        "fds-sm" ) pkg="fds-stormgr*.deb" ;;
        "fds-dm" ) pkg="fds-datamgr*.deb" ;;
        "fds-pm" ) pkg="fds-platformmgr*.deb" ;;
        *)
            logerror "[fdssetup] : unknown fds service [$service]"
            return 1
            ;;
    esac
    
    loginfo "[fdssetup] : installing $pkg"
    if [[ -e $pkg ]] ; then
        sudo dpkg -i $pkg
        echo ""
    else
        logerror "unable to locate : $pkg"
    fi
}



###########################################################################
# ------- MAIN PROGRAM --------
###########################################################################

for opt in "$@" ; do
    case $opt in 
        basepkgs) installBasePkgs ;;
        basedebs) installBaseDebs ;;
        python*) installPythonPkgs ;;
        fds-base*) installFdsBaseDebs ;;
        fds-*) installFdsService $opt ;;
        *) logerror "unknown option $opt" ;;
    esac
done

