#
# Cookbook Name:: collectd
# Recipe:: default
#
# Copyright 2014, YOUR_COMPANY_NAME
#
# All rights reserved - Do Not Redistribute
#

include_recipe "collectd::default"

packages = %w[
	golang
	git
]

packages.each do |pkg|
	package pkg
end

group node['influxdb_proxy']['proxy_groupname'] do
    action :create
end

user node['influxdb_proxy']['proxy_username'] do
    comment 'Account to run nodejs collectd-influxdb-proxy service'
    gid node['influxdb_proxy']['proxy_groupname']
    home "/home/#{node['influxdb_proxy']['proxy_username']}"
    supports :manage_home => true
    shell '/bin/bash'
    action :create
    system true
end

git "/opt/collectd-proxy" do
	repository "git://github.com/cboggs/influxdb-collectd-proxy.git"
	branch "master"
	action :sync
end

execute "fix_perms" do
	cwd "/opt"
	command "chown -R #{node['influxdb_proxy']['proxy_username']}:#{node['influxdb_proxy']['proxy_group']} collectd-proxy"
end

execute "build_proxy" do
	cwd "/opt/collectd-proxy"
	command "make"
	not_if { ::File.directory?("/opt/collectd-proxy/bin") }
	user node['influxdb_proxy']['proxy_username']
end

template "/etc/init/collectd-influxdb-proxy.conf" do
	source "collectd-influxdb-proxy-upstart.erb"
	variables ({
		:influxdb_host => node['influxdb_proxy']['influxdb_host'],
		:influxdb_port => node['influxdb_proxy']['influxdb_port'],
		:influxdb_db => node['influxdb_proxy']['influxdb_db'],
		:influxdb_user => node['influxdb_proxy']['influxdb_user'],
		:influxdb_password => node['influxdb_proxy']['influxdb_password'],
		:proxy_precision => node['influxdb_proxy']['proxy_precision'],
		:proxy_port => node['influxdb_proxy']['proxy_port'],
		:collectd_proxy_user => node['influxdb_proxy']['proxy_username'],
		:collectd_proxy_group => node['influxdb_proxy']['proxy_groupname']
	})
	notifies :restart, "service[collectd-influxdb-proxy]", :immediately
end

service "collectd-influxdb-proxy" do
	provider Chef::Provider::Service::Upstart
	action :start
end
