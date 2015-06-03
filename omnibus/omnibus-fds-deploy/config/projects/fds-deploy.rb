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

build_version "2015.05.22"
build_iteration 1

# Creates required build directories
dependency "preparation"

# fds-deploy dependencies/components
dependency "sshpass"
dependency "python"
dependency "pip"
dependency "boto"
dependency "ansible"
dependency "fds-deploy"

# Version manifest file
dependency "version-manifest"

exclude "**/.git"
exclude "**/bundler/git"
