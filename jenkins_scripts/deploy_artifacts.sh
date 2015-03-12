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
		if [ ! -z "${CHANNEL}" ]; then
			build_channel=${CHANNEL}
		else
			build_channel="nightly"
		fi

    if [ ! -z "${1}" ]; then
        pkg_filename=${1}
    else
        err "Must pass filename to function deploy_to_artifactory."
        return 1
    fi

    curl_status_code=$(curl -s -o /dev/null --write-out "%{http_code}" -XPUT "http://jenkins:UP93STXWFy5c@artifacts.artifactoryonline.com/artifacts/formation-apt/pool/${build_channel}/${pkg_filename};deb.distribution=platform;deb.component=${build_channel};deb.architecture=amd64" --data-binary @${pkg_filename})

    if [ ${curl_status_code} -ne 201 ]; then
        err "Upload failed, exiting"
    else
        log "Upload successful for ${pkg_filename}"
    fi
}

script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd -P)"
fds_platform_dir=(${script_dir}/../omnibus/omnibus-fds-platform/pkg ${script_dir}/../omnibus/omnibus-fds-deps/pkg)
pkgs_to_deploy=(
#    fds-platform
		fds-deps
)


fds_package_pattern='(fds-((platform-[a-z]+)|(deps))_[[:digit:]].[[:digit:]].[[:digit:]]-[[:alnum:]]+_amd64.deb)'
for dir in ${fds_platform_dir[@]}
do
	  echo "CD to $dir"
    cd $dir

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
done
