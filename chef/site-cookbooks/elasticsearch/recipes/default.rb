#
# Cookbook Name:: elasticsearch
# Recipe:: default
#
# Copyright 2014, YOUR_COMPANY_NAME
#
# All rights reserved - Do Not Redistribute
#

include_recipe "apt"

apt_repository 'elasticsearch' do
    uri         'http://packages.elasticsearch.org/elasticsearch/1.3/debian'
    components  ['stable', 'main']
    key         'http://packages.elasticsearch.org/GPG-KEY-elasticsearch'
end

package "elasticsearch" do
    action :install
end
