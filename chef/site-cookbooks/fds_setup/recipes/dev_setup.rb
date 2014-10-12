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
  libev-dev
  libfiu-dev
  fiu-utils
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
  libconfig++-dev
  fds-pkghelper
  fds-pkg
  fds-pkgtools
  fds-systemdir
  fds-coredump
  npm
  ruby
  ruby-sass
  python-paramiko
  python-redis
  python-requests
  python-boto
  python-argh
  python-thrift
  python-xmlrunner
]

fds_dev_python_packages = %w[
  scp
  PyYAML
  tabulate
]

apt_repository 'fds' do
  uri 'http://coke.formationds.com:8000'
  distribution 'fds'
  components [ 'test' ]
  key 'http://coke.formationds.com:8000/fds.repo.gpg.key'
end

apt_repository 'webupd8team' do
  uri 'http://ppa.launchpad.net/webupd8team/java/ubuntu'
  components [ 'trusty', 'main' ]
end

apt_repository 'redis-server' do
  uri 'http://ppa.launchpad.net/chris-lea/redis-server/ubuntu'
  components [ 'trusty', 'main' ]
end

package 'openjdk-7-jre' do
  action :purge
end
package 'openjdk-7-jre-headless' do
  action :purge
end

fds_dev_packages.each do |pkg|
  apt_package pkg do
      action :install
      options "--force-yes"
  end
end

fds_dev_python_packages.each do |pkg|
  python_pip pkg do
    action :install
  end
end

link "/usr/lib/jvm/java-8-oracle" do
  not_if "test -d /usr/lib/jvm/java-8-oracle"
  to "/usr/lib/jvm/java-8-oracle-amd64"
end

# group node['fds_setup']['fds_dev_groupname'] do
#     action :create
# end

# user node['fds_setup']['fds_dev_username'] do
#     comment 'FDS Admin service account'
#     gid node['fds_setup']['fds_dev_groupname']
#     home node['fds_setup']['fds_dev_homedir']
#     supports :manage_home => true
#     shell '/bin/bash'
#     password '$1$xyz$DOnIMQ/Uv9FXtWI/mUEC61'
#     action :create
#     system true
# end

# file "#{node['fds_setup']['fds_dev_homedir']}/clone-fds.sh" do
#     owner node['fds_setup']['fds_dev_username']
#     group node['fds_setup']['fds_dev_groupname']
#     mode '0755'
#     action :create_if_missing
#     content "git clone https://github.com/fds-dev/fds-src\ngit fetch origin dev\ngit checkout dev"
# end
