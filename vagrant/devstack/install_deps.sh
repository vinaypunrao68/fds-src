#!/bin/sh

sudo apt-get update && sudo apt-get -y install git vim-gtk libxml2-dev libxslt1-dev libpq-dev python-pip libsqlite3-dev mysql-server-5.5 && sudo apt-get -y build-dep python-mysqldb && sudo pip install git-review tox && git clone git://git.openstack.org/openstack-dev/devstack && cd devstack
