#!/bin/bash

function installAutoServices() {
    for service in ${AUTO_SERVICES} ; do
        update-rc.d ${service} defaults >/dev/null
    done
}

function removeAutoServices() {
    for service in ${AUTO_SERVICES} ; do
        update-rc.d ${service} remove >/dev/null
    done
}

function stopServices() {
    for service in ${SERVICES} ; do
        invoke-rc.d ${service} stop || exit $?
    done
}

function startServices() {
    for service in ${START_ON_INSTALL} ; do
        invoke-rc.d ${service} start || exit $?
    done
}

function processInstallScript() {
    local MODE=$1
    shift

    case ${MODE} in
        preinst)
            if [[ -n ${INSTALLSCRIPT} ]] ; then
                ${INSTALLSCRIPT} preinstall $@
            fi
            ;;

        postinst)
            if [[ -n ${INSTALLSCRIPT} ]] ; then
                ${INSTALLSCRIPT} postinstall $@
            fi
            installAutoServices
            startServices
            ;;

        prerm)
            stopServices
            if [[ -n ${INSTALLSCRIPT} ]] ; then
                ${INSTALLSCRIPT} preremove $@
            fi
            ;;
        
        postrm)
            if [ "$1" = "purge" ] ; then
                removeAutoServices
            fi
            if [[ -n ${INSTALLSCRIPT} ]] ; then
                ${INSTALLSCRIPT} postremove $@
            fi
            ;;

        *) 
            echo "unknown mode : ${MODE}"
            exit 1
            ;;
    esac
}
