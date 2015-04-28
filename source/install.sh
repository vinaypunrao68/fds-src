#!/bin/bash -le

# This script links various artifacts in the build tree and creates a FDS tree in ${DESTDIR}

BIN_LINK_FILE_LIST="bare_am DataMgr platformd StorMgr liborchmgr.so" 

SBIN_LINK_FILE_LIST="redis.sh"

LINK_DIRECTORY_LIST=""
 
[[ ${UID} -ne 0 ]] && { echo "This script must be executed with root privileges."; exit 1; }

if [[ ${#1} -gt 0 ]]
then
    DESTDIR=${1}
else
    DESTDIR='/fds'
fi

BINDIR=${DESTDIR}/bin
SBINDIR=${DESTDIR}/sbin
ETCDIR=${DESTDIR}/etc

JAVADIR=${DESTDIR}/lib/java

echo "Installing FDS into ${DESTDIR}"

for d in ${BINDIR} ${DESTDIR}/lib ${SBINDIR}
do
    mkdir -p ${d}
done

for f in ${BIN_LINK_FILE_LIST}
do
    echo "Checking ${f}"
    [[ -L ${BINDIR}/${f} ]] || { echo "linking ${f}"; ln -s ${PWD}/Build/linux-x86_64.debug/bin/${f} ${BINDIR}/${f}; }
done

for f in ${SBIN_LINK_FILE_LIST}
do
    echo "Checking ${f}"
    if [[ ! -f ${SBINDIR}/${f} ]] 
    then 
        if [[ -e ${PWD}/tools/${f} ]]
        then
            echo "linking ${f}"
            ln -s ${PWD}/tools/${f} ${SBINDIR}/${f};
        fi
    fi
done

echo "checking ${ETCDIR}"
[[ -L ${ETCDIR} ]] || { echo "linking ${ETCDIR}"; ln -s ${PWD}/config/etc/ ${ETCDIR}; }

echo "checking ${JAVADIR}"
[[ -L ${JAVADIR} ]] || { echo "linking ${JAVADIR}"; ln -s ${PWD}/Build/linux-x86_64.debug/lib/java/ ${JAVADIR}; }




