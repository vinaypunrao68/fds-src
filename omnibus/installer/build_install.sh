#!/bin/bash
# Run some pre-build steps

download=false

while getopts "d" opt; do
  case $opt in
    d)
      download=true
      ;;
  esac
done

ORGDIR="$(pwd)"
# Make sure we are in the build script dir
cd "$( dirname "${BASH_SOURCE[0]}" )"
ARTIFACTORY_URL="http://jenkins:UP93STXWFy5c@artifacts.artifactoryonline.com/artifacts/simple/formation-apt/pool/beta/"
DEPS="fds-deps_2015.06.19-32_amd64.deb"
PLATFORM="fds-platform-rel_2015.06.19-32_amd64.deb"
VERSION="$(cat ../../omnibus/VERSION)"
GIT_SHA="$(git rev-parse --short HEAD)"
INSTALLDIR="install_${VERSION}-${GIT_SHA}"

echo "Building installer for version ${VERSION}-${GIT_SHA}"

mkdir -p "${INSTALLDIR}/omnibus/omnibus-fds-platform/pkg"
mkdir -p "${INSTALLDIR}/omnibus/omnibus-fds-deps/pkg"
mkdir -p "${INSTALLDIR}/source/config/etc"
mkdir -p "${INSTALLDIR}/source/tools/install"
mkdir -p "${INSTALLDIR}/source/platform/python"
mkdir -p "${INSTALLDIR}/source/test"
mkdir -p "${INSTALLDIR}/ansible"

# If download is true - we download the artifacts vs. relying on them being local
if [ $download == "true" ] ; then
  # Download packages
  echo "Downloading fds-deps"
  deps_status_code="$(curl -o ${INSTALLDIR}/omnibus/omnibus-fds-deps/pkg/${DEPS} ${ARTIFACTORY_URL}/${DEPS})"
  echo "Downloading fds-platform"
  platform_status_code="$(curl -o ${INSTALLDIR}/omnibus/omnibus-fds-platform/pkg/${PLATFORM} ${ARTIFACTORY_URL}/${PLATFORM})"
  if [[ $deps_status_code -eq 0 && $platform_status_code -eq 0 ]] ; then
  	echo "All downloads successful"
  else
  	echo "One or more downloads failed"
  	exit 1
  fi
else
  echo "Syncing packages from local source"
  rsync -av ../../omnibus/omnibus-fds-platform/pkg/*.deb ${INSTALLDIR}/omnibus/omnibus-fds-platform/pkg/
  rsync -av ../../omnibus/omnibus-fds-deps/pkg/*.deb ${INSTALLDIR}/omnibus/omnibus-fds-deps/pkg/
fi

# Make sure we have package files
for fn in "platform" "deps" ; do
  if test -n "$(find ${INSTALLDIR}/omnibus/omnibus-fds-${fn}/pkg/ -maxdepth 1 -name '*.deb' -print -quit)" ; then
		echo "Package ${fn} found"
	else
		echo "Packages not found - aborting tarball creation!"
		exit 1
	fi
done

echo "Syncing ansible & configs"
rsync -a ../../source/config/etc/ ${INSTALLDIR}/source/config/etc/ --exclude="*.conf"
rsync -a ../../source/config/etc/fds_config_defaults.j2 ${INSTALLDIR}/source/test/
rsync -a ../../source/tools/install/fdsinstall.py ${INSTALLDIR}/source/tools/install/
rsync -a ../../source/platform/python/disk_type.py ${INSTALLDIR}/source/platform/python/
rsync -a ../../ansible/ ${INSTALLDIR}/ansible/ --exclude="**/resources" --exclude="**/inventory"
rsync -a install.sh ${INSTALLDIR}/
chmod 755 ${INSTALLDIR}/install.sh
rsync -a README.md ${INSTALLDIR}/

echo "Packaging installer"
tar cvfz ${INSTALLDIR}.tar.gz ${INSTALLDIR}
rm -rf ${INSTALLDIR}

cd $ORGDIR
