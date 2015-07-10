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
MEMCHECKDIR=${DESTDIR}/memcheck

JAVADIR=${DESTDIR}/lib/java

DATE_STAMP=`date +%s`

echo "Symlinking FDS into ${DESTDIR}"

for d in ${BINDIR} ${DESTDIR}/lib ${SBINDIR}
do
    mkdir -p ${d}
done

for f in ${BIN_LINK_FILE_LIST}
do
    [[ -e ${BINDIR}/${f} && ! -L  ${BINDIR}/${f} ]] &&  mv ${BINDIR}/${f} ${BINDIR}/${f}.${DATE_STAMP}
    [[ -L ${BINDIR}/${f} ]] && unlink ${BINDIR}/${f}

    echo "linking ${f}"
    ln -s ${PWD}/Build/linux-x86_64.debug/bin/${f} ${BINDIR}/${f}
done

for f in ${SBIN_LINK_FILE_LIST}
do
    [[ -e ${SBINDIR}/${f} && ! -L  ${SBINDIR}/${f} ]] &&  mv ${SBINDIR}/${f} ${SBINDIR}/${f}.${DATE_STAMP}
    [[ -L ${SBINDIR}/${f} ]] && unlink ${SBINDIR}/${f}

    if [[ ! -f ${SBINDIR}/${f} ]]
    then
        if [[ -e ${PWD}/tools/${f} ]]
        then
            echo "linking ${f}"
            ln -s ${PWD}/tools/${f} ${SBINDIR}/${f};
        fi
    fi
done

[[ -e ${ETCDIR} && ! -L ${ETCDIR} ]] && mv ${ETCDIR} ${ETCDIR}.${DATE_STAMP}
[[ -L ${ETCDIR} ]] && unlink ${ETCDIR}
echo "linking ${ETCDIR}"
ln -s ${PWD}/config/etc ${ETCDIR}

[[ -e ${MEMCHECKDIR} && ! -L ${MEMCHECKDIR} ]] && mv ${MEMCHECKDIR} ${MEMCHECKDIR}.${DATE_STAMP}
[[ -L ${MEMCHECKDIR} ]] && unlink ${MEMCHECKDIR}
echo "linking ${MEMCHECKDIR}"
ln -s ${PWD}/config/memcheck ${MEMCHECKDIR}

[[ -e ${JAVADIR} && ! -L ${JAVADIR} ]] && mv ${JAVADIR} ${JAVADIR}.${DATE_STAMP}
[[ -L ${JAVADIR} ]] && unlink ${JAVADIR}
echo "linking ${JAVADIR}"
ln -s ${PWD}/Build/linux-x86_64.debug/lib/java/ ${JAVADIR}
