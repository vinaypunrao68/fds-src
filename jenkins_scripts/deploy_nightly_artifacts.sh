#! /bin/bash

function log {
    echo "INFO :: ${1}"
}

function warn {
    echo
    echo "WARN :: ${1}"
}

function err {
    echo
    echo "ERR :: ${1}"
    exit 1
}

function deploy_to_artifactory () {
    if [ ! -z "${1}" ]; then
        pkg_filename=${1}
    else
        err "Must pass filename to function deploy_to_artifactory."
        return 1
    fi

    curl_status_code=$(curl -s -o /dev/null --write-out "%{http_code}" -XPUT "http://jenkins:UP93STXWFy5c@artifacts.artifactoryonline.com/artifacts/formation-apt/pool/nightly/${pkg_filename};deb.distribution=platform;deb.component=nightly;deb.architecture=amd64" --data-binary @${pkg_filename})

    if [ ${curl_status_code} -ne 201 ]; then
        err "Upload failed, exiting"
    else
        log "Upload successful for ${pkg_filename}"
    fi
}

script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd -P)"
fds_platform_dir=${script_dir}/../omnibus/omnibus-fds-platform/pkg
pkgs_to_deploy=(
    fds-platform
)


fds_package_pattern='(fds-[a-z]+_[[:digit:]].[[:digit:]].[[:digit:]]~nightly~(debug|release)-[[:alnum:]]+_amd64.deb)'
# ${BUILD_TYPE} should be "debug" or "release" and is provided by Jenkins
cd ${fds_platform_dir}

for pkg in ${pkgs_to_deploy[@]}
do
    log "---------------------"
    log "Checking  ::  ${pkg}"
    pkg_file=$(ls -l ${pkg}*.deb | awk '{ print $9 }')

    if [[ ${pkg_file} =~ ${fds_package_pattern} ]]; then
        log "${pkg_file}  ::  matches pattern, uploading to Artifactory" 
        deploy_to_artifactory ${pkg_file}
    else
        warn "${pkg_file} doesn't match pattern, skipping file"
    fi
done
