#!/bin/bash
# Run some pre-build steps

download=false
upload=false

function usage() {
  cat << EOF
Usage:
${0##/*} [Options]

Options:
  -d : Download .deb packages instead of using locally built artifacts
	-u : Upload installer tarball to artifactory
  -D : Name of fds-deps package to use (ex: fds-deps_2015.06.19-32_amd64.deb)
  -P : Name of fds-platform package to use (ex: fds-platform-rel_2015.06.19-32_amd64.deb)
  -h : Help

Note: -D & -P options may be env variables as \$DEPS and \$PLATFORM respectively
EOF
}

while getopts "duD:P:h" opt; do
  case $opt in
    d)
      download=true
      ;;
		u)
			upload=true
			;;
		D)
			DEPS=$OPTARG
			;;
		P)
			PLATFORM=$OPTARG
			;;
		h)
			usage
			exit 0
			;;
  esac
done

if [ ! -z "${CHANNEL}" ]; then
    build_channel=${CHANNEL}
else
    build_channel="nightly"
fi

ORGDIR="$(pwd)"
# Make sure we are in the build script dir
cd "$( dirname "${BASH_SOURCE[0]}" )"
ARTIFACTORY_URL="http://jenkins:UP93STXWFy5c@artifacts.artifactoryonline.com/artifacts/simple/formation-apt/pool/${build_channel}/"
ARTIFACTORY_INTERNAL_URL="http://jenkins:UP93STXWFy5c@bld-artifactory:8081/artifactory/simple/formation-apt/pool/${build_channel}/"
# If these aren't defined - we assign default values
DEPS="${DEPS:-fds-deps_2015.06.19-32_amd64.deb}"
PLATFORM="${PLATFORM:-fds-platform-rel_2015.06.19-32_amd64.deb}"
RABBITMQC="${RABBITMQC:-rabbitmq-c-0.7.1.deb}"
SIMPLEAMQPCLIENT="${SIMPLEAMQPCLIENT:-simpleamqpclient-0.7.1.deb}"
FDSSTATSCLIENT="${FDSSTATSCLIENT:-fds-stats-client-c-1.0.deb}"
STATSSERVICE="${STATSSERVICE:-fds-stats-service-0.0.1-SNAPSHOT.deb}"
STATSDEPS="${STATSDEPS:-fds-stats-deps_0.0.0+20150925040020-1_amd64.deb}"
VERSION="$(cat ../../omnibus/VERSION)"
BUILD_NUMBER="${BUILD_NUMBER:-~$(date +%s)}"
INSTALLDIR="install_${VERSION}-${BUILD_NUMBER}"
DLTMP="dltmp"

echo "Building installer for version ${VERSION}-${BUILD_NUMBER}"

mkdir -p "${INSTALLDIR}/omnibus/omnibus-fds-platform/pkg"
mkdir -p "${INSTALLDIR}/omnibus/omnibus-fds-deps/pkg"
mkdir -p "${INSTALLDIR}/source/config/etc"
mkdir -p "${INSTALLDIR}/source/tools/install"
mkdir -p "${INSTALLDIR}/source/platform/python"
mkdir -p "${INSTALLDIR}/source/test"
mkdir -p "${INSTALLDIR}/ansible"
mkdir -p "${INSTALLDIR}/omnibus/omnibus-fds-stats-service/pkg"
mkdir -p "${INSTALLDIR}/omnibus/omnibus-fds-stats-deps/pkg"
mkdir -p "${INSTALLDIR}/omnibus/omnibus-rabbitmq-c/pkg"
mkdir -p "${INSTALLDIR}/omnibus/omnibus-simpleamqpclient/pkg"
mkdir -p "${INSTALLDIR}/omnibus/omnibus-fds-stats-client-c/pkg"
mkdir -p "${DLTMP}"

# If download is true - we download the artifacts vs. relying on them being local
if [ $download == "true" ] ; then
  # Download packages
	for package in $DEPS $PLATFORM $RABBITMQC $SIMPLEAMQPCLIENT $FDSSTATSCLIENT $STATSSERVICE $STATSDEPS; do
		if [ ! -f ${DLTMP}/${package} ] ; then
			echo "Downloading $package"
			deps_status_code="$(curl -o ${DLTMP}/${package} ${ARTIFACTORY_URL}/${package})"
	  else
		  echo "Downloaded file ${package} already local - skipping download"
	  fi
	done
  rsync -av ${DLTMP}/${PLATFORM} ${INSTALLDIR}/omnibus/omnibus-fds-platform/pkg/
  rsync -av ${DLTMP}/${DEPS} ${INSTALLDIR}/omnibus/omnibus-fds-deps/pkg/
  rsync -av ${DLTMP}/${RABBITMQC} ${INSTALLDIR}/omnibus/omnibus-rabbitmq-c/pkg
  rsync -av ${DLTMP}/${SIMPLEAMQPCLIENT} ${INSTALLDIR}/omnibus/omnibus-simpleamqpclient/pkg
  rsync -av ${DLTMP}/${FDSSTATSCLIENT} ${INSTALLDIR}/omnibus/omnibus-fds-stats-client-c/pkg
  rsync -av ${DLTMP}/${STATSSERVICE} ${INSTALLDIR}/omnibus/omnibus-fds-stats-service/pkg
  rsync -av ${DLTMP}/${STATSDEPS} ${INSTALLDIR}/omnibus/omnibus-fds-stats-deps/pkg
else
  echo "Syncing packages from local source"
  rsync -av ../omnibus-fds-platform/pkg/*.deb ${INSTALLDIR}/omnibus/omnibus-fds-platform/pkg/
  rsync -av ../omnibus-fds-deps/pkg/*.deb ${INSTALLDIR}/omnibus/omnibus-fds-deps/pkg/
  rsync -av ../omnibus-fds-stats-service/pkg/*.deb ${INSTALLDIR}/omnibus/omnibus-fds-stats-service/pkg
  rsync -av ../omnibus-fds-stats-deps/pkg/*.deb ${INSTALLDIR}/omnibus/omnibus-fds-stats-deps/pkg
  rsync -av ../omnibus-rabbitmq-c/pkg/*.deb ${INSTALLDIR}/omnibus/omnibus-rabbitmq-c/pkg
  rsync -av ../omnibus-simpleamqpclient/pkg/*.deb ${INSTALLDIR}/omnibus/omnibus-simpleamqpclient/pkg
  rsync -av ../omnibus-fds-stats-client-c/pkg/*.deb ${INSTALLDIR}/omnibus/omnibus-fds-stats-client-c/pkg
fi

# Make sure we have package files
for fn in "platform" "deps" "stats-deps" "stats-service" "stats-client-c"; do
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

if [ $upload == "true" ] ; then
  echo "Uploading ${INSTALLDIR}.tar.gz to artifactory online"
  curl_status_code=$(curl -s -o /dev/null --write-out "%{http_code}" -XPUT "${ARTIFACTORY_URL}/${INSTALLDIR}.tar.gz" --data-binary @${INSTALLDIR}.tar.gz)
  if [ ${curl_status_code} -ne 201 ] ; then
      echo "Upload failed, exiting"
			exit 1
  else
      echo "Upload to artifactory online successful for ${INSTALLDIR}.tar.gz"
  fi
  echo "Uploading ${INSTALLDIR}.tar.gz to Boulder artifactory"
	curl_status_code=$(curl -s -o /dev/null --write-out "%{http_code}" -XPUT "${ARTIFACTORY_INTERNAL_URL}/${INSTALLDIR}.tar.gz" --data-binary @${INSTALLDIR}.tar.gz)
  if [ ${curl_status_code} -ne 201 ] ; then
      echo "Upload to Boulder Artifactory failed. Need to look at that."
  else
      echo "Upload to Boulder artifactory successful for ${INSTALLDIR}.tar.gz"
  fi
fi

cd $ORGDIR
