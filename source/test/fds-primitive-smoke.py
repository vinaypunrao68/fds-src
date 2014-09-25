#!/usr/bin/python
#!/usr/bin/python
#
# Copyright 2014 by Formation Data Systems, Inc.
#
import os, sys
import inspect
import argparse
import subprocess
import time

###
# Enumerations
#
class HTTPERROR:
    OK = 200
    NOT_FOUND = 404

REMOTE = False
CONFIG = None

##########################################################

###
# Fds test environment
#
class FdsEnv:
    def __init__(self, _root):
        self.env_cdir        = os.getcwd()
        self.srcdir          = ''
        self.env_root        = _root
        self.env_exit_on_err = True

        # assuming that this script is located in the test dir
        self.testdir  = os.path.dirname(
                            os.path.abspath(inspect.getfile(inspect.currentframe())))
        self.srcdir   = os.path.abspath(self.testdir + "/..")
        self.toolsdir = os.path.abspath(self.srcdir + "/tools")
        self.fdsctrl  = self.toolsdir + "/fds"
        self.cfgdir   = self.testdir + "/cfg"
        if CONFIG is not None:
            self.rem_cfg  = self.cfgdir + "/" + CONFIG
        else:
            self.rem_cfg  = self.cfgdir

        if self.srcdir == "":
            print "Can't determine FDS root from the current dir ", self.env_cdir
            sys.exit(1)

    def shut_down(self):
        if REMOTE:
            subprocess.call("python " + self.testdir + "/fds-tool.py -f " + self.rem_cfg + " -d", shell=True)
        else:
            subprocess.call([self.fdsctrl, 'stop', '--fds-root', self.env_root])

    def cleanup(self):
        self.shut_down()
        if REMOTE == False:
            subprocess.call([self.fdsctrl, 'clean', '--fds-root', self.env_root])
        else:
            subprocess.call("python " + self.testdir + "/fds-tool.py -f " + self.rem_cfg + " -c", shell=True)

###
# setup test environment, directories, etc
#
class FdsSetupNode:
    def __init__(self, fdsSrc, fds_data_path):
        try:
            os.mkdir(fds_data_path)
        except:
            pass
        subprocess.call(['cp', '-rf', fdsSrc + '/config/etc', fds_data_path])

#########################################################################################
###
# cluster bring up
#
def bringup_cluster(env, verbose, debug):
    env.cleanup()
    if REMOTE:
        print "python " + env.testdir + "/fds-tool.py -f " + env.rem_cfg + " -u"
        subprocess.call("python " + env.testdir + "/fds-tool.py -f " + env.rem_cfg + " -u", shell=True)
    else:
        root1 = env.env_root
        print "\nSetting up private fds-root in " + root1
        FdsSetupNode(env.srcdir, root1)

        os.chdir(env.srcdir + '/Build/linux-x86_64.debug/bin')

        print "\n\nStarting fds on ", root1
        subprocess.call([env.fdsctrl, 'start', '--fds-root', root1])
        os.chdir(env.srcdir)

###
# exit the test, shutdown if requested
#
def exit_test(env, shutdown):
    print "Test Passed, cleaning up..."

    if shutdown:
        env.cleanup()
    sys.exit(0)

#########################################################################################
###
# pre-commit test
#
def testsuite_pre_commit(am_ip):
    os.chdir(env.srcdir + '/Build/linux-x86_64.debug/tools')
    subprocess.call("./smokeTest " + am_ip, shell=True)

#########################################################################################
if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Start FDS Processes...')
    parser.add_argument('--cfg_file', default='./test/smoke_test.cfg',
                        help='Set FDS test cfg')
    parser.add_argument('--up', default='true',
                        help='Bringup system [true]')
    parser.add_argument('--down', default='true',
                        help='Shutdown/cleanup system after passed test run [true]')
    parser.add_argument('--smoke_test', default='false',
                        help='Run full smoke test [false]')
    parser.add_argument('--fds_root', default='/fds',
                        help='fds-root path [/fds')
    parser.add_argument('--am_ip', default='localhost',
                        help='AM IP address [localhost]')
    parser.add_argument('--verbose', default='false',
                        help='Print verbose [false]')
    parser.add_argument('--debug', default='false',
                        help ='pdb debug on [false]')
    parser.add_argument('--log_level', default=0,
                        help ='Sets verbosity of output.  Most verbose is 0, least verbose is 2')
    parser.add_argument('--std_output', default='yes',
                        help='Whether or not the script should print to stdout or to log files [no]')
    parser.add_argument('--config', default=None,
                        help='Name of config file to use (in test/cfg) for remote bringup')
    args = parser.parse_args()

    cfgFile      = args.cfg_file
    smokeTest    = args.smoke_test
    fds_root     = args.fds_root
    verbose      = args.verbose
    debug        = args.debug
    start_sys    = args.up
    am_ip        = args.am_ip
    log_level    = args.log_level
    config       = args.config

    global std_output
    std_output   = args.std_output;

    if args.down == 'true':
        shutdown = True
    else:
        shutdown = False

    ###
    # bring up the cluster
    #
    # Running remotely
    #if am_ip != 'localhost':
    #    REMOTE = True
    #    print 'Running smoke test remotely on IP %s' % am_ip
    if config is not None:
        global CONFIG
        assert(config != '')
        REMOTE = True
        CONFIG = config

    ###
    # load the configuration file and build test environment
    #
    env = FdsEnv(fds_root)

    if start_sys == 'true':
        bringup_cluster(env, verbose, debug)
        time.sleep(2)

    ###
    # run make precheckin and exit
    #
    if args.smoke_test == 'false':
        testsuite_pre_commit(am_ip)
        exit_test(env, shutdown)

    ###
    # clean exit
    #
    exit_test(env, shutdown)
