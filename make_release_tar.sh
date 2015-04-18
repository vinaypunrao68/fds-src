#!/bin/bash
# https://artifacts.artifactoryonline.com/artifacts/webapp/login.html?0
# fdsadmin - lastpast

# Beta Channel
CHANNEL=http://builder:h7qgA3fYr0P3@artifacts.artifactoryonline.com/artifacts/formation-apt/pool/beta
DEPS_DEB=fds-deps_0.7.5-1_amd64.deb
PLATFORM_DEB=fds-platform-rel_0.7.5-16_amd64.deb 

# Nightly Channel
#CHANNEL=http://builder:h7qgA3fYr0P3@artifacts.artifactoryonline.com/artifacts/formation-apt/pool/nightly
#DEPS_DEB=fds-deps_0.7.4-7_amd64.deb
#PLATFORM_DEB=fds-platform-rel_0.7.4-274_amd64.deb

DEPLOY_DEB=fds-deploy_0.7.5-1_amd64.deb

function remove_packages()
{
    echo "Removing existing packages"
    rm -f fds-deps_*_amd64.deb*
    rm -f fds-platform-*_amd64.deb*
    rm -f fds-deploy_*_amd64.deb*

    rm -f omnibus/omnibus-fds-deps/pkg/fds-deps_*_amd64.deb*
    rm -f omnibus/omnibus-fds-platform/pkg/fds-platform-*_amd64.deb*
    rm -f omnibus/omnibus-fds-deploy/pkg/fds-deploy_*_amd64.deb*

    rm -rf /tmp/openstack_package*
}

function download_packages()
{
    # XXX: deps is not in beta channel
    # wget $CHANNEL/$DEPS_DEB
    wget http://builder:h7qgA3fYr0P3@artifacts.artifactoryonline.com/artifacts/formation-apt/pool/nightly/$DEPS_DEB
    if [ $? -ne 0 ]; then
        echo "Get deps debian failed"
        exit 1
    fi
    mkdir -p omnibus/omnibus-fds-deps/pkg
    mv $DEPS_DEB omnibus/omnibus-fds-deps/pkg/


    wget $CHANNEL/$PLATFORM_DEB
    if [ $? -ne 0 ]; then
        echo "Get platform debian failed"
        exit 1
    fi
    mkdir -p omnibus/omnibus-fds-platform/pkg
    mv $PLATFORM_DEB omnibus/omnibus-fds-platform/pkg/
}

function build_packages()
{
    make package fds-deps
    make package fds-platform
}

function tar_all_packages()
{
    OPENSTACK_DEPLOY_SCRIPT=fds_deploy_openstack_driver.sh

    echo "Tar fds-deploy and openstack cinder driver"
    cp omnibus/omnibus-fds-deploy/pkg/$DEPLOY_DEB /tmp/
    cp source/openstack/$OPENSTACK_DEPLOY_SCRIPT /tmp/

    cd /tmp/
    tar czvf fds-deploy.tar.gz \
             $OPENSTACK_DEPLOY_SCRIPT \
             $DEPLOY_DEB \
             openstack_package.tar.gz
    cd -
}

#remove_packages

#download_packages
## OR
###build_packages

#make package fds-deploy
ls -l omnibus/omnibus-fds-platform/pkg
ls -l omnibus/omnibus-fds-deps/pkg
ls -l omnibus/omnibus-fds-deploy/pkg


# build openstack cinder driver packages
cd source/openstack
./make_package.sh redhat
cd -

tar_all_packages
