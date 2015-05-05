#!/bin/bash -le

# This script links various artifacts in the build tree and creates a FDS tree in ${DESTDIR}

BIN_LINK_FILE_LIST="bare_am DataMgr platformd StorMgr liborchmgr.so"

SBIN_LINK_FILE_LIST="redis.sh"

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

echo "Symlinking FDS into ${DESTDIR}"

for d in ${BINDIR} ${DESTDIR}/lib ${SBINDIR}
do
    mkdir -p ${d}
done

for f in ${BIN_LINK_FILE_LIST}
do
    [[ -L ${BINDIR}/${f} ]] && unlink ${BINDIR}/${f}

    echo "linking ${f}"
    ln -s ${PWD}/Build/linux-x86_64.debug/bin/${f} ${BINDIR}/${f}
done

for f in ${SBIN_LINK_FILE_LIST}
do
    [[ -L ${SBINDIR}/${f} ]] || unlink ${SBINDIR}/${f}

    if [[ ! -f ${SBINDIR}/${f} ]]
    then
        if [[ -e ${PWD}/tools/${f} ]]
        then
            echo "linking ${f}"
            ln -s ${PWD}/tools/${f} ${SBINDIR}/${f};
        fi
    fi
done

[[ -L ${ETCDIR} ]] && unlink ${ETCDIR}
echo "linking ${ETCDIR}"
ln -s ${PWD}/config/etc/ ${ETCDIR}

[[ -L ${JAVADIR} ]] && unlink ${JAVADIR}
echo "linking ${JAVADIR}"
ln -s ${PWD}/Build/linux-x86_64.debug/lib/java/ ${JAVADIR}




