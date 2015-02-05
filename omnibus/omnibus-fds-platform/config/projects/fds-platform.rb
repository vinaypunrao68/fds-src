#
# Copyright 2015 Formation Data Systems
#
# All Rights Reserved.
#

name "fds-platform"
maintainer "Cody Boggs (cody@formationds.com)"
homepage "www.formationds.com"

# Defaults to /opt/fds-platform
#install_dir "#{default_root}/#{name}"
install_dir "/fds"

fds_version = "0.7.0"
build_number = ENV['BUILD_NUMBER']
build_channel = ENV['BUILD_CHANNEL']
build_type = ENV['BUILD_TYPE']
git_sha = `git rev-parse --short HEAD`.chomp unless build_number

raise "BUILD_TYPE must be set to one of 'debug' or 'release'" unless build_type

fds_version << "~#{build_channel}" unless build_channel.nil?
fds_version << "~#{build_type}"

if build_number.nil?
    build_iteration git_sha
else
    build_iteration build_number
end


build_version fds_version

# Creates required build directories
dependency "preparation"

# fds-platform dependencies/components
dependency "fds-platform"

# Version manifest file
dependency "version-manifest"

exclude "**/.git"
exclude "**/bundler/git"
