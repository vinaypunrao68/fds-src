#
# Copyright 2015 YOUR NAME
#
# All Rights Reserved.
#

name "fds-deps"
maintainer "Formation Data Systems"
homepage "http://www.formationds.com"

fds_src_dir = ENV['FDS_SRC_DIR']
raise "FDS_SRC_DIR must be set'" unless fds_src_dir

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
  install_dir "/opt/fds/deps/#{fds_version}-#{git_sha}"
  name "fds-deps-#{fds_version}-#{git_sha}"
else
  install_dir "#{default_root}/#{name}"
  build_version fds_version
end

# Creates required build directories
dependency "preparation"

# fds-deps dependencies/components
dependency "python"
dependency "badger"
dependency "pip"
dependency "pcre"
dependency "boto"
dependency "paramiko"
dependency "pycrypto"
dependency "html5lib"
dependency "colorama"
dependency "distlib"
dependency "requests"
dependency "fstab"
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
dependency "mdadm"
dependency "influxdb"
dependency "fdsutil"
dependency "ansible"
dependency "python-httplib2"
dependency "sshpass"

# Version manifest file
dependency "version-manifest"

exclude "**/.git"
exclude "**/bundler/git"
