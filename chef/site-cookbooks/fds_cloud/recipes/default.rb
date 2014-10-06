#
# Cookbook Name:: fds_cloud
# Recipe:: default
#
# Copyright 2014, YOUR_COMPANY_NAME
#
# All rights reserved - Do Not Redistribute
#

include_recipe "sysctl::apply"

group node['fds_cloud']['fds-admin_groupname'] do
    action :create
end

user node['fds_cloud']['fds-admin_username'] do
    comment 'FDS Admin service account'
    gid node['fds_cloud']['fds-admin_groupname']
    home node['fds_cloud']['fds-admin_homedir']
    supports :manage_home => true
    shell '/bin/bash'
    password '$1$kGq4dO8c$ui5.xSaAWT43BhtVECd3S1'
    action :create
    system true
end

directory "#{node['fds_cloud']['fds-admin_homedir']}/.ssh" do
    owner node['fds_cloud']['fds-admin_username']
    group node['fds_cloud']['fds-admin_groupname']
    mode '0750'
    action :create
end

file "#{node['fds_cloud']['fds-admin_homedir']}/.ssh/authorized_keys" do
    owner node['fds_cloud']['fds-admin_username']
    group node['fds_cloud']['fds-admin_groupname'] 
    mode '0664'
    action :create_if_missing
    content node['fds_cloud']['fds-admin_authkey']
end
