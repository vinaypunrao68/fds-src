#
# Copyright 2015 YOUR NAME
#
# All Rights Reserved.
#

name "fds-deploy"
maintainer "Formation Data Systems"
homepage "http://www.formationds.com"

# Defaults to C:/fds-deploy on Windows
# and /opt/fds-deploy on all other platforms
install_dir "#{default_root}/#{name}"

mydir = File.dirname(__FILE__)
build_version = File.readlines("#{mydir}/../../../VERSION").first.chomp
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
