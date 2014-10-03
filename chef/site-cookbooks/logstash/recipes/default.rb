#
# Cookbook Name:: logstash
# Recipe:: default
#
# Copyright 2014, YOUR_COMPANY_NAME
#
# All rights reserved - Do Not Redistribute
#

include_recipe "apt"

apt_repository 'logstash' do
    uri         "http://packages.elasticsearch.org/logstash/#{node['logstash']['major_minor']}/debian"
    components  ['stable', 'main']
    key         'http://packages.elasticsearch.org/GPG-KEY-elasticsearch'
end

package "logstash" do
    action :install
    version "#{node['logstash']['major_minor']}.#{node['logstash']['patch']}"
end

node['logstash']['services_to_monitor'].each do |service|
    cookbook_file "/etc/logstash/conf.d/#{service}.conf" do
        action :create
        mode '0644'
        source "#{service}.conf" 
    end
end
