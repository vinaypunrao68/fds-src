#
# Copyright 2015 YOUR NAME
#
# All Rights Reserved.
#

name "fds-deploy"
maintainer "CHANGE ME"
homepage "https://CHANGE-ME.com"

# Defaults to C:/fds-deploy on Windows
# and /opt/fds-deploy on all other platforms
install_dir "#{default_root}/#{name}"

build_version Omnibus::BuildVersion.semver
build_iteration 1

# Creates required build directories
dependency "preparation"

# fds-deploy dependencies/components
dependency "sshpass"
dependency "python"
dependency "pip"
dependency "boto"
dependency "ansible"

# Version manifest file
dependency "version-manifest"

exclude "**/.git"
exclude "**/bundler/git"
