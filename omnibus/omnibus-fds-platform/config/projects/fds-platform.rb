#
# Copyright 2015 Formation Data Systems
#
# All Rights Reserved.
#

# Set the build type in the package name
if ENV['BUILD_TYPE'] == 'debug'
  build_type = 'dbg'
elsif ENV['BUILD_TYPE'] == 'release'
  build_type = 'rel'
else
  raise "BUILD_TYPE must be set to one of 'debug' or 'release'"
end

name "fds-platform-#{build_type}"
maintainer "Formation Data Systems"
homepage "http://www.formationds.com"


mydir = File.dirname(__FILE__)
fds_version = File.readlines("#{mydir}/../../../VERSION").first.chomp

build_number = ENV['BUILD_NUMBER']
git_sha = `git rev-parse --short HEAD`.chomp unless build_number

if build_number.nil?
  build_iteration git_sha
else
  build_iteration build_number
end

if ENV['ENABLE_VERSION_INSTALL'] == 'true'
  build_version "1"
  build_iteration "1"
  install_dir "/opt/fds/#{fds_version}-#{git_sha}"
  name "fds-platform-#{build_type}-#{fds_version}-#{git_sha}"
  # We override the build_iteration in this case
else
  # Continue to use legacy configuration
  install_dir "/fds"
  build_version fds_version
end

# Creates required build directories
dependency "preparation"

# fds-platform dependencies/components
dependency "fds-platform"

# Version manifest file
dependency "version-manifest"

exclude "**/.git"
exclude "**/bundler/git"
