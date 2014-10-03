#
# Cookbook Name:: fds_dev
# Recipe:: default
#
# Copyright (c) 2014 The Authors, All Rights Reserved.

include_recipe 'apt'
node.override[:java][:oracle][:accept_oracle_download_terms] = true
node.override[:java][:install_flavor] = "oracle"
node.override[:java][:jdk_version] = '8'
include_recipe 'java'

# Install all packages necessary for development
#
fds_dev_packages = %w[
  git
  linux-generic
  linux-headers-generic
  linux-image-generic
  python3-distupgrade
  ubuntu-release-upgrader-core
  gdb
  build-essential
  python-software-properties
  software-properties-common
  redis-server
  redis-tools
  maven
  libudev-dev
  libparted0-dev
  python2.7
  python-dev
  python-pip
  python-boto
  libpcre3-dev
  libpcre3
  libssl-dev
  libfuse-dev
  libevent-dev
  bison
  flex
  ragel
  ccache
  libboost-log1.54-dev
  libboost-program-options1.54-dev
  libboost-timer1.54-dev
  libboost-thread1.54-dev
  libboost-regex1.54-dev
  libical-dev
  libical1
  libleveldb-dev
  fds-pkghelper
  fds-pkg
  fds-pkgtools
  fds-systemdir
  fds-coredump
]

apt_repository 'fds' do
  uri 'http://coke.formationds.com:8000'
  distribution 'fds'
  components [ 'test' ]
  key 'http://coke.formationds.com:8000/fds.repo.gpg.key'
end

apt_repository 'webupd8team' do
  uri 'ppa:webupd8team/java'
  distribution node['lsb']['codename']
end

apt_repository 'redis-server' do
  uri 'ppa:chris-lea/redis-server'
  distribution node['lsb']['codename']
end

package 'openjdk-7-jre' do
  action :purge
end
package 'openjdk-7-jre-headless' do
  action :purge
end

fds_dev_packages.each do |pkg|
  package pkg
end
