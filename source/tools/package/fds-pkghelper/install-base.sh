#!/bin/bash

source /usr/include/pkghelper/loghelper.sh

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

function setupSymLinks() {
    if [[ -z ${SYMLINKS} ]]; then return ; fi

    local num=${#SYMLINKS[@]}
    local i,target, link

    for (( i=0; i < num ; i+=2 )) ; do
        target="${SYMLINKS[$i]}"
        link="${SYMLINKS[$((i+1))]}"
        if [[ -e $target ]] ; then
            loginfo "[fdsinstall] : setting up link [$link] --> [$target]"
            if ! (ln -s $target $link) ; then
                logerror "[fdsinstall] : symlink failed - mebbe it already exists : [$link]"
            fi
        else
            logerror "[fdsinstall] : symlink failed , [$target] does not exist"
        fi
    done
}

function removeSymLinks() {
    if [[ -z ${SYMLINKS} ]]; then return ; fi

    local num=${#SYMLINKS[@]}
    local i,target, link

    for (( i=0; i < num ; i+=2 )) ; do
        target="${SYMLINKS[$i]}"
        link="${SYMLINKS[$((i+1))]}"

        if [[ -e $link ]] ; then
            loginfo "[fdsinstall] : removing symlink link [$link] --> [$target]"
            if [[ -h $link ]] ; then
                rm -f $link
            else
                logerror "[fdsinstall] : remove symlink failed, [$link] is not a symlink"
            fi
        else
            logerror "[fdsinstall] : remove symlink failed , [$link] does not exist"
        fi
    done
}



function processInstallScript() {
    local MODE=$1
    shift

    case ${MODE} in
        preinst)
            if [[ -n ${INSTALLSCRIPT} ]] ; then
                ${INSTALLSCRIPT} preinstall "$@"
            fi
            ;;

        postinst)
            if [[ -n ${INSTALLSCRIPT} ]] ; then
                ${INSTALLSCRIPT} postinstall "$@"
            fi
            setupSymLinks
            installAutoServices
            startServices
            ;;

        prerm)
            stopServices
            if [[ -n ${INSTALLSCRIPT} ]] ; then
                ${INSTALLSCRIPT} preremove "$@"
            fi
            ;;
        
        postrm)
            if [ "$1" = "purge" ] ; then
                removeAutoServices
                removeSymLinks
            fi

            if [[ -n ${INSTALLSCRIPT} ]] ; then
                ${INSTALLSCRIPT} postremove "$@"
            fi
            
            ;;

        *) 
            echo "unknown mode : ${MODE}"
            exit 1
            ;;
    esac
}
