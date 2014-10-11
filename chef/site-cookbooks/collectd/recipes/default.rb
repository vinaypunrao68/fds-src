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

service "collectd" do
	action [:enable, :start]
end
