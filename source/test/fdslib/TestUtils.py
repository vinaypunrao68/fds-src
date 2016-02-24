#!/usr/bin/python
#
# Copyright 2014 by Formation Data Systems, Inc.
#
import pdb
import os
import sys
import time
import logging
import logging.handlers
import errno

from optparse import OptionParser
if sys.version_info[0] < 3:
    import ConfigParser as configparser
else:
    import configparser

import BringUpCfg as fdscfg
import socket
from fdscli.services.fds_auth import *
from fdscli.model.volume.settings.object_settings import ObjectSettings
from fdscli.model.volume.settings.block_settings import BlockSettings
from fdscli.model.volume.settings.nfs_settings import NfsSettings
from fdscli.model.common.size import Size
from fdscli.model.volume.volume import Volume
from fdscli.services.volume_service import VolumeService
from fdscli.model.volume.qos_policy import QosPolicy
from fdscli.services.node_service import NodeService
from fdscli.services.local_domain_service import LocalDomainService
from fabric.contrib.files import *
from fabric.context_managers import cd
import fnmatch
import fabric
import fabric.network

# Assumes tests run from /source/test/testsuites
RELPATH_TEMPLATES="../testsuites/templates/"
RELPATH_DEVROOT="../../../"
TESTSUITES_INVENTORY="%sansible-inventory/" % RELPATH_TEMPLATES
DEFAULT_INVENTORY="%sansible/inventory/" % RELPATH_DEVROOT

def _setup_logging(log_name, dir, log_level, max_bytes=100*1024*1024, rollover_count=5):
    # Set up the core logging engine
    log = logging.getLogger()
    log.setLevel(log_level)
    logging.addLevelName(100, "REPORT")

    log_fmt = logging.Formatter("%(asctime)s %(name)-24s %(levelname)-8s %(message)s",
                                "%Y-%m-%d %H:%M:%S")

    screen_handler = logging.StreamHandler()
    screen_handler.setLevel(logging.ERROR)
    screen_handler.setFormatter(log_fmt)
    log.addHandler(screen_handler)

    # Set up the log file and log file rotation
    if not os.path.isdir(dir):
        os.makedirs(dir)

    log_file = os.path.join(dir, log_name)

    if not os.access(dir, os.W_OK):
       log.warning("There is no write access to the specified log "
                         "directory %s  No log file will be created." %dir)
    else:
        log_handle = None
        try:
            log_handle = logging.handlers.RotatingFileHandler(log_file, "w",
                                                              max_bytes,
                                                              rollover_count)
            log_handle.setLevel(log_level)
            log_handle.setFormatter(log_fmt)
            log.addHandler(log_handle)
            if os.path.exists(log_file):
                log_handle.doRollover()

        except Exception as e:
            log.error(e.message)
            log.error("Failed to rollover the log file. Perhaps a permissions problem on the log file %s." %
                      log_file)
            logging.getLogger("").removeHandler(log_handle)
            sys.exit(1)

    log.info("Logfile: %s" % log_file)

    return log

def get_options(pyUnit):
    parser = OptionParser()
    parser.prog = sys.argv[0].split("/")[-1]
    parser.usage = "%prog -h | <Suite|.csv|File:Class> \n" + \
                   "<-q <qaautotest.ini> | -s <test_source_dir>> " + \
                   "[-b <build_num>] \n[-l <log_dir>] [--level <log_level>] [-z |--inventory-file <inventory_file>]" + \
                   "[--stop-on-fail] [--run-as-root] \n" + \
                   "[--iterations <num_iterations>] [--store] \n" + \
                   "[-v|--verbose] [-y|--reusecluster] [-r|--dryrun]> [-i|--install]>"

    # FDS: Changed to option i/ini_file from c/config to prevent clashing with PyUnit's c/catch option.
    parser.add_option("-q", "--qat-file", action="store", type="string",
                      dest="config", help="The file containing the test harness "
                      "configuration information. (qaautotest's .ini file.)")
    parser.add_option("-s", "--src-dir", action="store", type="string",
                      dest="test_source_dir", help="The path to the directory "
                      "in which the tests may be found.")
    parser.add_option("-b", "--build", action="store", type="string",
                      dest="build", default="Unspecified", help="The build "
                      "number of the code under test.")
    #FDS: Option -l used to have default='.' but this overrode configuration .ini settings.
    parser.add_option("-l", "--log-dir", action="store", type="string",
                      dest="log_dir", help="The directory in "
                      "which the log file should be located.  If unspecified, "
                      "the current directory will be used.")
    parser.add_option("--level", action="store", type="string",
                      dest="log_level", help="The log level for the harness "
                      "logger, e.g. logging.DEBUG, 20, etc.")
    parser.add_option("--threads", action="store", type="int",
                      default=1, dest="threads", help="If unset, the tests "
                      "will be run serially, i.e. in a single-threaded mode.  "
                      "To run tests in parallel, set the number of threads to "
                      "use.  Each test case will be run in its own thread.")
    parser.add_option("--iterations", action="store", type="int",
                      dest="iterations", help="If unset, the specified tests "
                      "will be run once.  Otherwise, the tests are repeated "
                      "the specified number of iterations.  Enter -1 for "
                      "infinite.")
    parser.add_option("--stop-on-fail", action="store_true",
                      dest="stop_on_fail", help="If set, the harness will stop "
                      "immediately if a test run fails.")
    parser.add_option("--run-as-root", action="store_true",
                      dest="run_as_root", help="If set, the test cases will "
                      "all be run as the root user.")
    parser.add_option("--store", action="store_true",
                      dest="store_to_database", help="If set, the results of "
                      "the test run will be archived in the test database.")
    # FDS: Add a couple of FDS-specific options.
    parser.add_option('-v', '--verbose', action = 'store_true', dest = 'verbose',
                      help = 'enable verbosity')
    parser.add_option('-y', '--reusecluster', action = 'store_true', dest = 'reusecluster',
                      help = 'Dont deploy or teardown domain, use existing nodes and then clean them instead of uninstalling')
    parser.add_option('-r', '--dryrun', action = 'store_true', dest = 'dryrun',
                      help = 'dry run, print commands only')
    parser.add_option('-i', '--install', action = 'store_true', dest = 'install',
                      help = 'perform an install from an FDS package as opposed '
                                              'to a development environment')
    parser.add_option('-d', '--sudo-password', action = 'store', dest = 'sudo_password',
                      help = 'When the node is localhost, use this password for sudo access to the configured user. ')

    parser.add_option("-z","--inventory-file", action="store", type="string", dest = 'inventory_file',
                      help = 'If given , will over ride ip addresses in cfg file with inventory ips')

    validate_cli_options(parser, pyUnit)
    return parser

def validate_cli_options(parser, pyUnit):
    (options, args) = parser.parse_args()
    if (len(args) == 0) and (pyUnit == False):
        parser.error("The first argument must be the test name, the suite "
                     "name, or .csv file in which a list of tests can be found.")
    if len(args) > 1:
        parser.error("Multiple positional arguments have been specified.")
    if (options.store_to_database == True and options.build == "Unspecified") and (pyUnit == False):
        parser.error("To store test results to the database, the build number "
                     "must be specified.  If you do not have a build number, "
                     "enter the date, e.g. 2010-09-17.")
    try:
        if int(options.iterations) == -1:
            options.iterations = pow(2, 24) # infinite enough
    except (ValueError, TypeError):
        pass

def get_config(pyUnit = False, pyUnitConfig = None, pyUnitVerbose = False, pyUnitDryrun = False, pyUnitInstall = False, pyUnitSudoPw = None, pyUnitInventory = None, pyUnitReuseCluster = False):
    """ Configuration can be gathered from one of two sources: 1) a
    configuration .ini file and/or 2) the command line.  Configuration settings
    will first be imported from the file, if the option has been specified.
    Then, command line options will be applied.  When specified in both places,
    command line arguments will override those defined in the configuration
    file. FDS: So be careful about what options have defaults. In those cases, the
    value in the configuration.ini will be ignored even if *not* specified
    on the command line.
    Following the import of the configuration, the settings will be scrubbed.
    Strings will be converted to int or bool where appropriate.  Paths on
    the file system will be resolved to their absolute paths.
    Note that if the file system is already mounted, the multicast IP, export,
    and mount point will be taken from the already mounted file system,
    irrespective of what has been specified in the configuration .ini file or
    the command line.
    FDS: With pyUnit = True, we understand that we are being called in support
    of a test being run with Python's unittest module infrastructure.
    """

    # In the event we try to log messages before we've set up logging...
    logging.basicConfig(level=logging.ERROR,
                format='%(asctime)s (%(thread)d) %(name)-24s %(levelname)-8s %(message)s',
                datefmt='%Y-%m-%d %H:%M:%S')

    # Initialize variables.
    # Import all options passed in at the command line.
    params = {}

    # Set up a child PID dictionary among the parameters.
    # Key will be the name of the scenario in which a child
    # was forked. Value is the child's PID.
    params["child_pid"] = {}

    # FDS: We must have this config file specified.
    params["fds_config_file"] = None

    parser = get_options(pyUnit)
    (options, args) = parser.parse_args()
    if pyUnit == False:
        try:
            params["tests"] = args[0]
        except IndexError:
            print("The first argument must be the test name, the suite name, "
                  "or .csv file in which a list of tests can be found.")
            sys.exit(1)
    else:
        # When run under PyUnit, we expect the config file to be passed
        # in as pyUnitConfig. But it should be the same as the .ini
        # file used by a qaautotest run.
        options.config = pyUnitConfig

    # Process configuration parameters specified in the configuration .ini
    # file.
    if options.config != None:
        if not os.path.exists(options.config):
            print("The qaautotest configuration .ini file specified doesn't exist: %s"
                  %options.config)
            sys.exit(1)
        config = configparser.SafeConfigParser()
        try:
            config.read(options.config)
        except configparser.MissingSectionHeaderError:
            print("The configuration file %s is not in the expected "
                  "format.  It contains no section headers."
                  %options.config)
            sys.exit(1)
        try:
            print("Importing configuration settings from file: %s"
                  %options.config)
            for item in config.items("harness"):
                params[item[0]] = item[1].split()[0]
        except configparser.NoSectionError:
            print("The configuration file %s has no section \"[harness]\".  "
                  "This section is required." % options.config)
            sys.exit(1)

    # Process parameters defined at the command line.  These will override
    # anything set in the .ini file.
    for key, value in options.__dict__.items():
        if key in params and params[key] == None:
            params[key] = value
        elif key in params and params[key] == "":
            params[key] = value
        elif key in params and value != None:
            params[key] = value
        if key not in params:
            params[key] = value

    # Apply some sanity conversions.  str->int and str->bool
    # Also, make all path parameters absolute paths
    for key, value in params.items():
        try:
            if isinstance(params[key], str):
                params[key] = int(value)
        except (ValueError, TypeError):
            pass
        if value == "True":
            params[key] = True
        elif value == "False":
            params[key] = False
        if isinstance(params[key], str):
            if (key == "test_source_dir") or (key == "log_dir") or (key == "config"):
                if params[key][0] == '~':
                    params[key] = os.path.expanduser(params[key])
                elif params[key][0] != '/':
                    params[key] = os.path.abspath(params[key])

                #params[key] = os.path.abspath(value)

    # Ensure the number of iterations is valid
    try:
        if not isinstance(params["iterations"], int):
            params["iterations"] = 1
    except KeyError:
        params["iterations"] = 1

    # Check also the logging option to make sure it's in integer format:
    level_options = {"critical": 50,
                     "error": 40,
                     "warning": 30,
                     "info": 20,
                     "debug": 10,
                     "notset": 0}
    if not isinstance(params["log_level"], int):
        try:
            if "logging" in params["log_level"]:
                p = params["log_level"].split(".")[1].lower()
            else:
                p = params["log_level"].lower()
            params["log_level"] = level_options[p]
        except (TypeError, KeyError):
            params["log_level"] = logging.DEBUG

    logging.getLogger().setLevel(params["log_level"])

    # The test directory, i.e. the place where the test
    # code may be found, must be specified.
    if not pyUnit:
        if params["test_source_dir"] is None:
            print("The test directory is not specified.  Add the -s option to "
                  "your command line or add the mount item to the configuration "
                  ".ini file.")
            sys.exit(1)
        elif not os.path.isdir(params["test_source_dir"]):
            print("The test directory specified, test_source-dir=%s, does not exist." % params["test_source_dir"])
            sys.exit(1)


    # FDS: The log directory, i.e. the place where the test
    # logs may be written, must be specified.
    if params["log_dir"] is None:
        if pyUnit:
            print("The log directory is not specified.  Add the log_dir item to "
                  "your config file: %s." % options.config)
        else:
            print("The log directory is not specified.  Add the -l option to "
                  "your command line or add the log_dir item to the configuration "
                  ".ini file.")
        sys.exit(1)

    # FDS: Check the passed options in case we are running from PyUnit
    if "verbose" in params:
        if params["verbose"] == False:
            params["verbose"] = pyUnitVerbose
        setattr(options, "verbose", params["verbose"])
    else:
        setattr(options, "verbose", False)

    if "dryrun" in params:
        if params["dryrun"] == False:
            params["dryrun"] = pyUnitDryrun
        setattr(options, "dryrun", params["dryrun"])
    else:
        setattr(options, "dryrun", False)

    if "sudo_password" in params:
        if params["sudo_password"] is None:
            params["sudo_password"] = pyUnitSudoPw
        setattr(options, "sudo_password", params["sudo_password"])
    else:
        setattr(options, "sudo_password", "dummy")

    if "inventory_file" in params:
        if params["inventory_file"] is None:
            if pyUnitInventory is None:
                params["inventory_file"] = "generic-lxc-nodes"
            else:
                params["inventory_file"] = pyUnitInventory
        setattr(options,"inventory_file", params["inventory_file"])
    else:
        setattr(options, "inventory_file", "generic-lxc-nodes")

    global run_as_root
    if params["run_as_root"] == True:
        run_as_root = True
    else:
        run_as_root = False

    # FDS: We must have an FDS config file specified in the qaautotest .ini file for the suite.
    if params["fds_config_file"] is None:
        print("You need to pass item fds_config_file in your qaautotest test suite .ini file: %s"
                  %options.config)
        sys.exit(1)
    setattr(options, "config_file", params["fds_config_file"])

    if not os.path.isfile(options.config_file):
        print("The fds_config_file item, %s, in your qaautotest test suite .ini file does not exist." %
              options.config_file)
        sys.exit(1)

    # FDS: This one is not required, particularly if we want to run from
    # an installation package.
    if params["fds_source_dir"] is None:
        params["fds_source_dir"] = ''
    setattr(options, "fds_source_dir", params["fds_source_dir"])

    # FDS: Some FDS root directory must be specified to satisfy dependencies in FdsConfigRun.
    # But it will be correctly set when the FDS config file is parsed for each identified node.
    #
    # The first parameter to FdsConfigRun is an FdsEnv which will be determined
    # once we have the FDS root directory right.
    setattr(options, "fds_root", '.')
    params["fdscfg"] = fdscfg.FdsConfigRun(None, options, test_harness=True)

    # FDS: Now record whether we are being run by PyUnit.
    params["pyUnit"] = pyUnit

    # Currently, 3/2/2015, if we do an Ansible package install,
    # we only want to do it once as it will install the software
    # on all identified nodes at once.
    params["ansible_install_done"] = pyUnitInstall

    return params

def findNodeFromInv(node_inventory, target):
    '''
    Looks for target in node_inventory and returns the target object.
    :param node_inventory: A list of nodes to search through (typically nd_confg_dict)
    :param target: Target node to find (a parameter passed into the test case)
    :return: Returns the node object identified by target
    '''

    # If we didn't get a string, just return the original object
    if not isinstance(target, str):
        return target

    # Otherwise look for a node with the target name
    for node in node_inventory:
        if node.nd_conf_dict['node-name'] == target:
            return node

    # else return None for debugging purposes
    return "None"

def check_localhost(ip):
    ipad = socket.gethostbyname(ip)
    if ipad.count('.') == 4:
        hostName = socket.gethostbyaddr(ipad)[0]
    else:
        hostName = ip

    if (hostName == 'localhost') or (ipad == '127.0.0.1') or (ipad == '127.0.1.1'):
        return True
    else:
        return False

def create_fdsConf_file(om_ip):
    fileName = os.path.join(os.path.expanduser("~"), ".fdscli.conf")
    file = open(fileName, "w")
    writeString = '[toggles]\ncommand_history=true\n[connection]\nhostname='+om_ip+'\nusername=admin\npassword=admin\nport=7777\n'
    file.write(writeString)
    file.close()

def convertor(volume, fdscfg):
    new_volume = Volume()
    new_volume.name=volume.nd_conf_dict['vol-name']
    new_volume.id=volume.nd_conf_dict['id']

    if 'media' not in volume.nd_conf_dict:
        media = 'hdd'
    else:
        media = volume.nd_conf_dict['media']
    new_volume.media_policy =media.upper()

    if 'access' not in volume.nd_conf_dict or volume.nd_conf_dict['access'] == 'object':
        #set default volume settings to ObjectSettings
        access = ObjectSettings()
    else:
        #if its a block then set the size in BlockSettings
        access = volume.nd_conf_dict['access']
        if access == 'block':
            if 'size' not in volume.nd_conf_dict:
                raise Exception('Volume section %s must have "size" keyword.' % volume.nd_conf_dict['vol-name'])
            access = BlockSettings()
            access.capacity = Size( size = volume.nd_conf_dict['size'], unit = 'B')
        elif access == 'nfs':
            if 'size' not in volume.nd_conf_dict:
                raise Exception('Volume section %s must have "size" keyword.' % volume.nd_conf_dict['vol-name'])
            access = NfsSettings()
            access.max_object_size = Size( size = volume.nd_conf_dict['size'], unit = 'B')

    new_volume.settings = access
    if 'policy' in volume.nd_conf_dict:
        #Set QOS policy which is defined is volume definition.
        new_volume.qos_policy = get_volume_policy(volume.nd_conf_dict['policy'], fdscfg)

    return new_volume


def get_inventory_value(inventory_file, key_name, log):
    '''
    Parse the given Ansible inventory file given as argument to this
    function, and find a value for the given key

    Arguments:
    ----------
    inventory_file : str
        The name of the Ansible inventory file to be parsed
    key_name : str
        A key that may or may not exist in the inventory file
    log : 
        A logger

    Returns:
    --------
    str or None : the value for the given key or None if key not found
        If duplicate keys, returns the value for the first instance found
    '''
    result = None
    if not key_name:
        log.error("Missing required argument");
        raise Exception
    if not isinstance(key_name, str):
        log.error("Invalid argument");
        raise Exception
    inventory_path = os.path.join(TESTSUITES_INVENTORY, inventory_file)
    if not os.path.isfile(inventory_path):
        # Fall back to default inventory location
        inventory_path = os.path.join(DEFAULT_INVENTORY, inventory_file)
        if not os.path.isfile(inventory_path):
            log.error('Inventory file {0} not found'.format(inventory_path))
            raise Exception

    with open(inventory_path, 'r') as f:
        records = f.readlines()
        for record in records:
            if record.startswith(key_name):
                result = record.strip().split("=")[1]
                break
    return result


def get_volume_service(self,om_ip):
    getAuth(self, om_ip)
    return VolumeService(self.__om_auth)

def get_volume_policy(policy_id, fdscfg):
    qos_policy = QosPolicy()
    policies = fdscfg.rt_get_obj('cfg_vol_pol')
    for policy in policies:
        if 'id' not in policy.nd_conf_dict:
            print('Policy section must have an id')
            sys.exit(1)

        if policy.nd_conf_dict['id'] == policy_id:
            if 'iops_min' in policy.nd_conf_dict:
                qos_policy.iops_min = policy.nd_conf_dict['iops_min']

            if 'iops_max' in policy.nd_conf_dict:
                qos_policy.iops_max = policy.nd_conf_dict['iops_max']

            if 'priority' in policy.nd_conf_dict:
                qos_policy.priority = policy.nd_conf_dict['priority']

    return qos_policy

def get_node_service(self, om_ip):
    getAuth(self,om_ip)
    return NodeService(self.__om_auth)

def get_localDomain_service(self, om_ip):
    getAuth(self,om_ip)
    return LocalDomainService(self.__om_auth)

def getAuth(self, om_ip):
    create_fdsConf_file(om_ip)
    file_name = os.path.join(os.path.expanduser("~"), ".fdscli.conf")
    self.__om_auth = FdsAuth(file_name)
    print "Attempting to authenticate to %s" % (om_ip)
    retryCount = 0
    maxRetries = 20
    while retryCount < maxRetries:
      retryCount += 1
      try:
        self.__om_auth.login()
        break

      except Exception as e:
        if retryCount < maxRetries:
          retryTime = 1 + ( (retryCount - 1) * 0.5 )
          time.sleep(retryTime)
        else:
          raise FdsAuthError(message="Login unsuccessful, OM is down or unreachable.", error_code=404)

        continue

def read_ips_from_tmp(inventory_file_name):
    filepath = '/tmp/'+inventory_file_name+'_ips.txt'
    f = open(filepath, "r")
    contents = f.readlines()
    f.close()
    contents[0]=contents[0].replace('OM_HOST','')
    ips_array = []

    for i in contents:
        ips_array.append(i.strip())

    return ips_array

def node_is_up(self,om_ip,node_id):
    node_service = get_node_service(self,om_ip)
    node = node_service.get_node(node_id)
    if node.state == 'UP':
        return True
    else:
        return False

def deploy_on_AWS(self, number_of_nodes, inventory_file):
    deploy_script = 'deploy_fds_ec2.sh'
    deb_location = 'local'
    deploy_script_dir = os.path.join(self.rt_env.env_fdsSrc, '../ansible/scripts/')
    cur_dir = os.getcwd()
    os.chdir(deploy_script_dir)
    cmd = './%s %s %s %s' %(deploy_script,inventory_file, number_of_nodes, deb_location)
    status = os.system(cmd)
    os.chdir(cur_dir)
    if status != 0:
        self.log.error("FDS package installation on AWS nodes returned status %d." %
                           (status))
        return False

    return True

def core_hunter_aws(self,node_ip):
    connect_fabric(self, node_ip)
    if exists('/fds/bin', use_sudo=True):
        for dir in {'/fds/bin','/fds/var/log/corefiles'}:
            with cd(dir):
                files = run('ls').split()
                for file in files:
                    if fnmatch.fnmatch(file, "*.core") or fnmatch.fnmatch(file, "*.hprof") or fnmatch.fnmatch(file,"*hs_err_pid*.log"):
                        fabric.state.connections[node_ip].get_transport().close()
                        self.log.error("Core file %s detected at node %s:%s"%(file,node_ip,dir))
                        return 0
    disconnect_fabric()
    return 1

# Returns a path to the named resource. Does not validate that the resource exists.
def get_resource(self, resource):

    fdscfg = self.parameters["fdscfg"]
    resourceDir = fdscfg.rt_env.get_fds_source() + "test/testsuites/resources/"

    self.log.debug("Retrieving resource {}.".format(resourceDir + resource))

    return resourceDir + resource
    
def connect_fabric(self,node_ip):
    """Specify connection info at runtime

    Fabric is a library of subroutines to make executing shell commands over SSH
    easy and Pythonic.

    Parameters
    ----------
    self : obj
    node_ip : str
    """
    # 'env' is a global dictionary-like object driving many of Fabric's settings.
    env.user = get_inventory_value(self.parameters['inventory_file'],'fds_ssh_user', self.log)
    env.password = get_inventory_value(self.parameters['inventory_file'],'fds_ssh_password', self.log)
    # 'host_string' defines the current user/host/port which Fabric will connect
    # to when executing run, put, and so forth.
    env.host_string = node_ip
    timeout_start = time.time()
    timeout = 600  # Max 10 minutes wait considering bare metal/ pxe reboot
    while time.time() < timeout_start + timeout:
        try:
            internal_ip = run("hostname")
        except Exception as e:
            # Sleep for 20 sec before retrying to connect node
            time.sleep(20)
            continue
        else:
            sudo("echo '127.0.0.1 %s' >> /etc/hosts" % internal_ip)
            return True

    self.log.error('Node %s unreachable after 10 mins retry time'%node_ip)
    return False


def disconnect_fabric():
    fabric.network.disconnect_all()


# This method returns occurrences of 'log_entry' in `service*.log` on node `node_ip`
def read_remote_log(self, node_ip, service, log_entry):
    assert connect_fabric(self, node_ip) is True
    with cd('/fds/var/logs'):
        files = run('ls').split()
    log_files = [item for item in files if item.startswith(service + '.log')]

    log_counts = 0
    io = StringIO()
    for log_file in log_files:
        get('/fds/var/logs/' + log_file, io)
        content = io.getvalue()
        search_lines = content.split('\n')
        for line in search_lines:
            if log_entry in line:
                log_counts += 1
                io.truncate(0)

    disconnect_fabric()
    return log_counts


# This method returns dictionary of passed log_entry_list with respective occurence count
# on given node_ip. Will search in respective service in service_list
def get_log_count_dict(self, om_node_ip, node_ip, service_list, log_entry_list):
    log_count_dict = {}
    for index, log_entry in enumerate(log_entry_list):
        node_ip = om_node_ip if service_list[index] == 'om' else node_ip
        val = read_remote_log(self, node_ip, service_list[index], log_entry)
        log_count_dict[log_entry] = val
    return log_count_dict


# A pseudo random SHA-1 generator.
def sha1_generator(seed='seed'):
    next_sha = hashlib.sha1(seed)

    while True:
        yield next_sha.digest(), next_sha.digestsize
        next_sha = hashlib.sha1(next_sha.hexdigest())


default_generated_file = "./generated.dat"  # Used in the generate_file() and remove_file() methods below.


# A pseudo random file generator that generates the named file of the given
# size using the given seed.
#
# @param size In bytes.
def generate_file(qualified_file_name=default_generated_file, size=1024, seed='seed'):
    with open(qualified_file_name, 'w') as generated_file:
        bytes_written = 0
        for next_content_block, block_size in sha1_generator(seed=seed):
            bytes_to_write = block_size
            if bytes_written + bytes_to_write > size:
                bytes_to_write = size - bytes_written

            content_to_write = bytearray(buffer(next_content_block, 0, bytes_to_write))
            generated_file.write(content_to_write)

            bytes_written += bytes_to_write

            if (bytes_written >= size):
                break

    return generated_file.closed


# Remove a file if it exists.
def remove_file(qualified_file_name=default_generated_file):
    try:
        os.remove(qualified_file_name)
    except OSError as e:
        if e.errno != errno.ENOENT:
            raise
