#
# Cookbook Name:: collectd
# Recipe:: default
#
# Copyright 2014, YOUR_COMPANY_NAME
#
# All rights reserved - Do Not Redistribute
#

include_recipe "apt"

packages = %w[
	collectd
	collectd-utils
]

packages.each do |pkg|
	package pkg
end

template "/etc/collectd/collectd.conf" do
    source "collectd.conf.erb"
    variables ({
        :proxy_host => node['influxdb_proxy']['proxy_host'],
        :proxy_port => node['influxdb_proxy']['proxy_port']
    })
	notifies :restart, "service[collectd]", :immediately
end

group "collectd" do
    action :create
end

user "collectd" do
    comment 'collectd account'
    gid "collectd"
    home "/home/collectd"
    supports :manage_home => true
    shell '/bin/bash'
    action :create
    system true
end

directory "/var/log/collectd" do
	action :create
	mode '0755'
	owner 'nobody'
	group 'nogroup'
end	

file "/var/log/collectd/collectd.log" do
	action :create
	mode '0766'
	owner 'nobody'
	group 'nogroup'
end	

cookbook_file "/home/collectd/get_cpu_core_count.sh" do
	source "get_cpu_core_count.sh"
	action :create
	owner 'collectd'
	group 'collectd'
    mode '0755'
end

service "collectd" do
	action [:enable, :start]
end
