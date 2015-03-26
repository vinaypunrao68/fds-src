#!/usr/bin/python
# Copyright 2014 by Formation Data Systems, Inc.
# Written by Philippe Ribeiro
# philippe@formationds.com
test_list = "fast_scale_test.json"
log_dir = "test-reports"
test_sets = "testsets"
setup = "setup.cfg"

FDS_ROOT="/root/fds-src/"
FDS_TOOLS="../../../source/tools/"
FDS_CMD="sudo %s/fds %s"
pyUnitConfig = "./../../source/test/testsuites/BuildSmokeTest.ini"

S3_USER = 'fds_testing'
S3_PASSWORD = 'bIEYKCPzDKtQDDBomnAKTKRA'

FDS_DEFAULT_ADMIN_USER        = 'admin'
FDS_DEFAULT_ADMIN_PASS        = 'admin'
FDS_DEFAULT_HOST              = '127.0.0.1'

FDS_SSL_PORT                  = 443
FDS_REST_PORT                 = 7777
FDS_S3_PORT                   = 8443
FDS_DEFAULT_IS_SECURE         = True

FDS_DEFAULT_BUCKET_NAME       = "demo_volume2"
FDS_DEFAULT_BUCKET_PREFIX     = "demo_volume"
FDS_DEFAULT_KEY_NAME          = "file_1"
FDS_DEFAULT_KEYS_COUNT        = 10
FDS_DEFAULT_FILE_PATH         = "/tmp/foobar"
TEST_DEBUG                    = 1

SSH_USER                      = 'root'
SSH_PASSWORD                  = 'passwd'
STOP_ALL_CLUSTER              = "./fds-tool.py -f %s -d -c"
START_ALL_CLUSTER             = "./fds-tool.py -f %s -u"
FDS_SBIN                      = "/fds/sbin"
FORMATION_CONFIG              = "deploy_formation.conf"

NDBADM_CLIENT                 = "10.1.16.101"
FDS_MOUNT                     = "/fdsmount"
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

NUMBER_USERS = 30
MAX_NUM_VOLUMES = 1023
SAMPLE_FILE = "test.txt"
SAMPLE_DIR = "./samples/"
RANDOM_DATA = "./random_data/"
DD = "dd if=/dev/urandom of=%s bs=%s count=1"
TEST_DIR = "./test_files/"
BACKUP_DIR = "./backup/"
REPOSITORY_URL = "http://coke.formationds.com/jenkins_tests/samples.tar.gz"
DOWNLOAD_DIR = "./downloads/"
ROOT = "/root/"
ANSIBLE_ROOT = "../../../ansible/"
ANSIBLE_INVENTORY = "%s/inventory/" % ANSIBLE_ROOT
EC2_TEMPLATE = "./templates/ec2"
BAREMETAL_TEMPLATE = "./templates/scale-framework-cluster"
NDBADM = "../../../source/cinder/nbdadm.py"
VAULT_PASS = ".vault_pass.txt"
DESTROY_EC2 = "scripts/teardown_ec2_cluster.sh %s"
START_EC2 = "scripts/deploy_fds_ec2.sh %s %s %s"
START_BAREMETAL = "scripts/deploy_fds.sh %s %s"

LOCAL_CLUSTER_IP_TABLE = ("10.2.10.200", "10.2.10.201", "10.2.10.202", 
                          "10.2.10.203")

CONFIG_DIR = "./config/"
SYSTEM_FRAMEWORK = "testsuites"
SYSTEM_CMD = "../%s/DomainBootSuite.py -q ../%s/StaticMigration.ini -d dummy --verbose"
DEFAULT_INVENTORY_FILE = "scale-framework-cluster"
CMD_CONFIG = ['./setup_module.py', '-q', './BuildSmokeTest.ini', '-d', 'dummy', '--verbose']
