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
    
    local symarray=(${SYMLINKS[@]})
    local num=${#symarray[@]}
    local i
    local target
    local link

    for (( i=0; i < num ; i+=2 )) ; do
        target="${symarray[$i]}"
        k=$((i+1))
        link="${symarray[$k]}"
        #echo "target=$target"
        #echo "link=$link"
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

    local symarray=(${SYMLINKS[@]})
    local num=${#symarray[@]}
    local i
    local target
    local link

    for (( i=0; i < num ; i+=2 )) ; do
        target="${symarray[$i]}"
        link="${symarray[$((i+1))]}"

        if [[ -h $link ]] ; then
            loginfo "[fdsinstall] : removing symlink link [$link] --> [$target]"
            rm -f $link
        else
            logerror "[fdsinstall] : remove symlink failed , [$link] does not exist"
        fi
    done
}

function runldconfig() {
    if [[ ${PKG_HAS_SOLIBS} == '1' ]] ; then
        loginfo "[fdsinstall] : running ldconfig to update soname bindings"
        ldconfig
    fi
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
            runldconfig
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
            removeAutoServices
            removeSymLinks
            runldconfig
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
