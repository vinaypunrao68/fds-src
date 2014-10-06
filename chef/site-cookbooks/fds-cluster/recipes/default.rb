#
# Cookbook Name:: fds-cluster
# Recipe:: default
#
# Copyright (c) 2014 Formation, All Rights Reserved.

#include_recipe 'apt'

user "root" do
  password "$1$UXeYJGm3$w1VWCoJeY5nQ4jFJ8QDq5."
  action :modify
  not_if { node.attribute?("root_password_complete") }
end

ruby_block "root_password_run_flag" do
  block do
    node.set['root_password_complete'] = true
    node.save
  end
  action :nothing
end

bash "install_fds" do
  user "root"
  cwd "/"
  code <<-EOH
  tar -zxf #{node[:installer_dir]}/fdsinstall.tar.gz
  cd fdsinstall
  ./fdsinstall.py -o 2
  ./fdsinstall.py -o 3
  ./fdsinstall.py -o 4
  sed -i 's/om_ip = "127.0.0.1"/om_ip = "#{node[:om_ip]}"/g' /fds/etc/orch_mgr.conf
  sed -i 's/om_ip            = "127.0.0.1"/om_ip            = "#{node[:om_ip]}"/g' /fds/etc/platform.conf
  sed -i 's/node1_ip/#{node[:node1_ip]}/g' /fds/sbin/formation.conf
  sed -i 's/node2_ip/#{node[:node2_ip]}/g' /fds/sbin/formation.conf
  sed -i 's/node3_ip/#{node[:node3_ip]}/g' /fds/sbin/formation.conf
  sed -i 's/node4_ip/#{node[:node4_ip]}/g' /fds/sbin/formation.conf
  /fds/sbin/redis.sh start
  EOH
  not_if { node.attribute?("install_fds_complete") }
end

ruby_block "install_fds_run_flag" do
  block do
    node.set['install_fds_complete'] = true
    node.save
  end
  action :nothing
end
