#!/bin/bash -ex

#####################################################
# Get thrift from artifactory
######################################################

artifactoryPullUser="builder"
artifactoryPullPassword="h7qgA3fYr0P3"

thrift_version="0.9.0"
thrift_fds_versioned="fds-thrift-${thrift_version}"
thrift_tar_gz="${thrift_fds_versioned}.tar.gz"

thrift_debug_lib_dir="Build/linux-x86_64.debug/lib"
thrift_release_lib_dir="Build/linux-x86_64.release/lib"
thrift_include_dir="Build/thrift-${thrift_version}/lib/cpp/src"

thrift_debug_bin_dir="Build/linux-x86_64.debug/bin"
thrift_release_bin_dir="Build/linux-x86_64.release/bin"


make_directories_if_needed()
{
  mkdir -p "${thrift_debug_lib_dir}"
  mkdir -p "${thrift_release_lib_dir}"
  mkdir -p "${thrift_include_dir}"
  mkdir -p "${thrift_debug_bin_dir}"
  mkdir -p "${thrift_release_bin_dir}"
}

##################################

make_directories_if_needed

# Overwrite libthrift.a only when it does not exist in fds-src/Build/linux-x86_64.debug/lib.
if [ ! -e "${thrift_debug_lib_dir}/libthrift.a"  ]; then
  # wget -nc will only download if the tar.gz file does not exist
   wget -nc --user "${artifactoryPullUser}" --password "${artifactoryPullPassword}"  http://artifacts.artifactoryonline.com/artifacts/third-party-deps/thrift-${thrift_version}/dist/${thrift_tar_gz} --directory-prefix "${thrift_debug_lib_dir}"

   tar -xzf "${thrift_debug_lib_dir}/${thrift_tar_gz}" -C "${thrift_debug_lib_dir}"  "${thrift_fds_versioned}/lib/libthrift.a" --strip=2
   tar -xzf "${thrift_debug_lib_dir}/${thrift_tar_gz}" -C "${thrift_release_lib_dir}"  "${thrift_fds_versioned}/lib/libthrift.a" --strip=2
   tar -xzf "${thrift_debug_lib_dir}/${thrift_tar_gz}" -C "${thrift_include_dir}"  "${thrift_fds_versioned}/include"  --strip=2
   tar -xzf "${thrift_debug_lib_dir}/${thrift_tar_gz}" -C "${thrift_debug_bin_dir}"  "${thrift_fds_versioned}/bin"  --strip=2
   tar -xzf "${thrift_debug_lib_dir}/${thrift_tar_gz}" -C "${thrift_release_bin_dir}"  "${thrift_fds_versioned}/bin"  --strip=2
fi


exit $?