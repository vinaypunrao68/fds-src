#!/usr/bin/python
# Copyright 2014 by Formation Data Systems, Inc.
# Written by Philippe Ribeiro
# philippe@formationds.com
test_list = "test_list.json"
log_dir = "test-reports"
test_sets = "testsets"
setup = "setup.cfg"

params = {
    'log_level' : 20,
    'stop_on_fail' : True,
    'run_as_root' : False,
    'iterations' : 1,
    'threads' : 1,
    'root_user' : 'root',
    'sudo_password' : 'passwd',
    'host' : '1.2.3.4',
    'num_nodes' : 10,
    'pyUnit' : True,
    'tar_file' : False,
    'build' : None,
    'dryrun' : False,
    'store_to_database' : False,
    'verbose' : False,
}