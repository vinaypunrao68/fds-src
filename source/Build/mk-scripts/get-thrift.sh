#!/bin/bash -ex

TOPDIR=$(git rev-parse --show-toplevel)

#####################################################
# Get thrift from artifactory
######################################################

artifactoryPullUser="builder"
artifactoryPullPassword="h7qgA3fYr0P3"

thrift_version="0.9.3"
thrift_fds_versioned="fds-thrift-${thrift_version}"
thrift_tar_gz="${thrift_fds_versioned}.tar.gz"

thrift_dir_prefix="${TOPDIR}/Build"
thrift_lib_dir="${thrift_dir_prefix}/${thrift_fds_versioned}/lib"

##################################

_success_=0
# Overwrite libthrift.a only when it does not exist in fds-src/Build/linux-x86_64.debug/lib.
if [ ! -e "${thrift_lib_dir}/libthrift.a"  ]; then
  # wget -nc will only download if the tar.gz file does not exist
   wget -nc --user "${artifactoryPullUser}" --password "${artifactoryPullPassword}" \
       http://artifacts.artifactoryonline.com/artifacts/third-party-deps/thrift-${thrift_version}/dist/${thrift_tar_gz} \
       --directory-prefix "${thrift_dir_prefix}"

   # Note: explicitly exclude .so until fds can successully link against it
   # Package also contains html doc.  Not currently unpacking that here.
   tar -xzf "${thrift_dir_prefix}/${thrift_tar_gz}" -C "${thrift_dir_prefix}" \
       --exclude="*.so.*" \
       --exclude="${thrift_fds_versioned}/share"

   _success_=$?
else
    echo "Using existing FDS thrift package at ${thrift_dir_prefix}"
fi

exit ${_success_}
