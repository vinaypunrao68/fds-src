#
# Copyright 2015 YOUR NAME
#
# All Rights Reserved.
#

name "fds-deps"
maintainer "Formation Data Systems"
homepage "http://www.formationds.com"

# Defaults to C:/fds-deps on Windows
# and /opt/fds-deps on all other platforms
install_dir "#{default_root}/#{name}"

#build_version Omnibus::BuildVersion.semver
build_version '0.7.0'
build_iteration 1

# Creates required build directories
dependency "preparation"

# fds-deps dependencies/components
dependency "python"
dependency "pip"
dependency "pcre"
dependency "boto"
dependency "paramiko"
dependency "pycrypto"
dependency "html5lib"
dependency "colorama"
dependency "distlib"
dependency "parted"
dependency "libconfig"
dependency "libev"
dependency "readline"
dependency "util-linux"
dependency "xfsprogs"
dependency "libjemalloc"
dependency "libical"
dependency "libhiredis"
dependency "redis"
dependency "libevent"
dependency "libicu"
dependency "libboost1_55"
dependency "libunwind"
dependency "google-perftools"
dependency "leveldb"
dependency "pycurl"
dependency "python-scp"
dependency "libcryptopp"
dependency "server-jre"

# Version manifest file
dependency "version-manifest"

exclude "**/.git"
exclude "**/bundler/git"
