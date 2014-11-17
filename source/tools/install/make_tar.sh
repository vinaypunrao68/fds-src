#!/bin/bash -le

package_basename="fdsinstall-"
package_tailname=".tar.gz"

if [[ ${#1} -gt 0 ]]
then
   dest_tarfile=${1}
else
   build_stamp="`date +%Y%m%d-%H%M`"
   dest_tarfile="${package_basename}${build_stamp}${package_tailname}"
fi


if [[ ${#BUILD_NUMBER} -gt 0 ]]
then
   package_path="./jenkins"
   pkg_make_args="release-build=false"
else
   package_path="/tmp"
fi

install_path="${package_path}/fdsinstall"

if [[ ${#JENKINS_BUILD} -eq 0 ]]
then
   echo "Cleaning up previous packaging tree"
   rm -rf ${install_path} ${package_path}/${package_basename}*${package_tailname}
fi

echo "Making packages"
cd pkg
make ${pkg_make_args}

cd ..
make DEST_PATH="${install_path}"

echo "Packaging install files into ${dest_tarfile}"
cd ${package_path}
tar czvf ${dest_tarfile} fdsinstall

echo "Complete, filename:  ${dest_tarfile}"
