#!/bin/bash -le

source ./loghelper.sh

if [[ ${#1} -gt 0 ]]
then
   INSTALLDIR=${1}
else
   INSTALLDIR=/tmp/fdsinstall
fi


SOURCEDIR=$(pwd)
PKGDIR=$(cd pkg; pwd)
PKGSERVER=http://coke.formationds.com:8000/external/
THISFILE=$(basename $0)
PKGCACHEDIR="${SOURCEDIR}/.pkg.cache"

[[ "${INSTALLDIR:0:1}" == "." ]] && INSTALLDIR=${SOURCEDIR}/${INSTALLDIR}

externalpkgs=(
    libpcre3_8.31-2ubuntu2_amd64.deb
    libconfig++9_1.4.9-2_amd64.deb
    libev4_4.15-3_amd64.deb
    libreadline5_5.2+dfsg-2_amd64.deb

    xfsprogs_3.1.9ubuntu2_amd64.deb
    java-common_0.51_all.deb
    oracle-java8-jdk_8u5_amd64.deb

    libical1_1.0-0ubuntu1_amd64.deb

    libjemalloc1_3.6.0.deb
    redis-server_2.8.17.deb
    redis-tools_2.8.17.deb
    libhiredis0.10_0.11.0-3_amd64.deb

    libical1_1.0-0ubuntu1_amd64.deb

    paramiko-1.15.1.tar.gz
    scp-0.8.0.tar.gz
    boto-2.34.0.tar.gz
    crypto-1.0.1.tar.gz
    Naked-0.1.30-py2.py3-none-any.whl
    ecdsa-0.11.tar.gz

    python-pip_1.5.4-1_all.deb
    python-html5lib_0.999-2_all.deb
    python-distlib_0.1.8-1_all.deb
    python-colorama_0.2.5-0.1ubuntu1_all.deb
    python-setuptools_3.3-1ubuntu1_all.deb
)

fdscorepkgs=(
    fds-coredump
    fds-platform
)

fdsrepopkgs=(
    fds-pkghelper
    fds-boost
    fds-leveldb
    fds-jdk-default
    fds-python-scp
    fds-python-paramiko
)

function getExternalPkgs() {
    if [[ ! -e ${PKGCACHEDIR} ]] ; then
        loginfo "Creating package cache directory [${PKGCACHEDIR}]"
        mkdir -p ${PKGCACHEDIR}
    fi

    loginfo "Checking external pkgs"
    (
        cd ${INSTALLDIR}
        for pkg in ${externalpkgs[@]} ; do
            loginfo "Checking pkg [$pkg]"
            if [[ ! -f ${PKGCACHEDIR}/${pkg} ]] ; then
                loginfo "fetching pkg [$pkg]"
                if ! (wget --no-verbose ${PKGSERVER}/$pkg -O ${PKGCACHEDIR}/${pkg}) ; then
                    logerror "unable to fetch [$pkg] from ${PKGSERVER}"
                fi
            fi
            loginfo "copying ${pkg}"
            cp ${PKGCACHEDIR}/${pkg} ${pkg}
        done
    )
}

function getFdsCorePkgs() {
    loginfo "copying core fds pkgs"
    (
        cd ${PKGDIR}
        for pkg in ${fdscorepkgs[@]} ; do
            debfile=$(ls -1t ${pkg}*.deb | head -n1)
            if [[ -n $debfile ]]; then
                loginfo "copying ${debfile}"
                cp ${debfile} ${INSTALLDIR}
            else
                logerror "unable to locate .deb file for [$pkg] in ${PKGDIR}"
            fi
        done
    )
}

function getFdsRepoPkgs() {
    loginfo "fetching fds pkgs from repo"
    sudo fdspkg update
    (
        cd ${INSTALLDIR}
        for pkg in ${fdsrepopkgs[@]} ; do
            loginfo "fetching $pkg from fds pkg repo"
            if ! (apt-get download $pkg) ; then
                logerror "unable to fetch [$pkg] from fds pkg repo"
            fi
        done
    )
}


function getInstallSources() {
    loginfo "copying install sources"
    ( 
        cd ${SOURCEDIR}
        for file in *.py *.sh ../../platform/python/disk_type.py; do
            if [[ ${file} != ${THISFILE} ]] && [[ ${file} != "make_tar.sh" ]]; then
                loginfo "copying [$file] to ${INSTALLDIR}"
                cp $file ${INSTALLDIR}
            fi
        done
    )
}

function saveGitCommitNumber() {
    echo `git status | grep "On branch"` > ${INSTALLDIR}/git_commit.txt
    echo >> ${INSTALLDIR}/git_commit.txt
    echo `git log -n 1` >> ${INSTALLDIR}/git_commit.txt
}

function setupInstallDir() {
    loginfo "creating install dir @ ${INSTALLDIR}"
    if [[ ! -e ${INSTALLDIR} ]]; then
        mkdir -p ${INSTALLDIR}
    fi

    getExternalPkgs
    getFdsRepoPkgs
    getFdsCorePkgs
    getInstallSources
    saveGitCommitNumber
}

#############################################################################
# Main program
#############################################################################

setupInstallDir
#getFdsCorePkgs
