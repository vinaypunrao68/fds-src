#!/bin/bash -le

package_basename="fdsinstall-"
build_stamp="`date +%Y%m%d-%H%M`"
package_tailname=".tar.gz"

dest_tarfile="${package_basename}${build_stamp}${package_tailname}"

if [[ ${#BUILD_NUMBER} -gt 0 ]]
then
   package_path="./jenkins"
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
make

cd ..
make DEST_PATH="${install_path}"

echo "Packaging install files into ${dest_tarfile}"
cd ${package_path}
tar czvf ${dest_tarfile} fdsinstall

echo "Complete, filename:  ${dest_tarfile}"
