#!/bin/bash
echo pip uninstall thrift
pip uninstall -y thrift

echo yum -y remove python-pip sshpass
yum -y remove python-pip sshpass
