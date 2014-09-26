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
REPOUPDATED=0
DEPLOYMODE=0
INSTALLFDSPKGS=0

function progress() {
    if isDeployMode ; then
        echo "[deploysetup] : $*"
    else
        echo "[devsetup] : $*"
    fi
}

needed_packages=(
    redis-server
    oracle-java8-installer oracle-java8-set-default maven
    libudev-dev 
    libparted0-dev
    python2.7
    python-dev
    python-pip
    python-boto

    libpcre3-dev
    libpcre3
    libssl-dev
    libfuse-dev

    libevent-dev

    bison
    flex
    ragel
    ccache
    clang

    libboost-log1.54-dev
    libboost-program-options1.54-dev
    libboost-timer1.54-dev
    libboost-thread1.54-dev
    libboost-regex1.54-dev

    libical-dev
    libical1

    npm
    ruby
    ruby-sass

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
    boto
)

function isDeployMode() {
    [[ $DEPLOYMODE == 1 ]]
}

function shouldInstallFdsPkg() {
    [[ $INSTALLFDSPKGS == 1 ]]
}

function isFdsPkg() {
    local pkg="$1"
    case "$pkg" in
        fds*) true ;;
        *) false ;;
    esac
}

###########################################################################
# returns  if the passed in pkg is a dev pkg
###########################################################################
function isDevPackage() {
    local pkg="$1"
    case "$pkg" in
        *-dev) true ;;
        paramiko) true ;;
        redis) true ;; # python pkg
        requests) true;;
        scp) true;;
        PyYAML) true;;
        bison|flex|ragel|ccache) true;;
        fds-pkgtools) true;;
        *) false;;
    esac
}

###########################################################################
# update the fds repo as needed
###########################################################################
function updateFdsRepo() {
    if [[ $REPOUPDATED == "0" ]]; then
        sudo apt-get update -o Dir::Etc::sourcelist="sources.list.d/fds.list" -o Dir::Etc::sourceparts="-" -o APT::Get::List-Cleanup="0"
        REPOUPDATED=1
    fi
}

##########################################################################
# check and ensure the software-properties-common package is installed
# this is necessary for the add-apt-repository command used in preinstall 
##########################################################################
function checkSWPropsCommon() {
    pkgname=software-properties-common
    pkginfo=$( dpkg-query -f '${Package}' -W $pkgname 2>/dev/null)
    if [[ -z $pkginfo ]]; then
        sudo apt-get install $pkgname --assume-yes
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

function installBasePkgs() {
    progress "checking ubuntu packages...."

    for pkg in ${needed_packages[@]} 
    do 
        if isDeployMode && isDevPackage $pkg ; then
            progress "-- skipping dev pkg : $pkg"
            continue
        fi

        if isFdsPkg $pkg && ! shouldInstallFdsPkg ; then
            progress "-- skipping FDS pkg : $pkg"
            continue
        fi

        progress "checking $pkg ..."

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
                progress "$pkgname is in unknown state [$pkgstate] - forcing install"
                pkginfo=
            fi
        fi

        #echo "${pkginfo} , $pkgname, $pkgversion , $pkg"
        #exit
        if  [[ -z $pkginfo ]] || [[ $pkginfo != $pkg ]] ; then 
            if [[ $pkgversion == "latest" ]] ; then
                progress "trying to install the latest version of $pkgname ..."
            else
                progress "$pkg is not installed, but needed .. installing."
            fi
            preinstall $pkgname
            if [[ -z $pkgversion ]] || [[ $pkgversion == "latest" ]] ; then
                sudo apt-get install ${pkgname} --assume-yes
            else
                sudo apt-get install ${pkgname}=${pkgversion} --assume-yes
            fi
            postinstall $pkgname
        fi
    done

}

function installPythonPkgs() {
    progress "checking python packages...."

    for pkg in ${python_packages[@]} 
    do
        if isDeployMode && isDevPackage $pkg ; then
            progress "-- skipping dev pkg : $pkg"
            continue
        fi

        if isFdsPkg $pkg && ! shouldInstallFdsPkg ; then
            progress "-- skipping FDS pkg : $pkg"
            continue
        fi

        pkgname=${pkg}
        progress "[python] checking : $pkg"
        name=$(pip freeze 2>/dev/null | grep ^${pkgname}= | cut -f1 -d=)
        if  [[ -z $name ]] ; then 
            progress "$pkg is not installed, but needed .. installing."
            preinstall $pkg
            sudo pip install $pkg
            postinstall $pkg
        fi
    done
}

function postInstallSetup() {
    if [[ ! -e /usr/bin/node ]]
    then
        sudo ln -s /usr/bin/nodejs /usr/bin/node
    fi
}

###########################################################################
# ------- MAIN PROGRAM --------
###########################################################################
case $1 in
    *deploymode) DEPLOYMODE=1 ;;
    *fdspkg*) INSTALLFDSPKGS=1;;
esac

checkSWPropsCommon
installBasePkgs
installPythonPkgs
postInstallSetup
