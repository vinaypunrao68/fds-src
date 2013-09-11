#
# Copyright 2013 Formation Data Systems, Inc.
#

import random
import unittest
import optparse
import subprocess
import psutil
import shutil
import os
import re
import pdb
import sys

#
# Components
# Uses hackey enums for components
# 
INVALID = 0
STORMGR = 1
DATAMGR = 2
STORHVI = 3
VCC = 4
UBD = 5
components = [STORMGR, DATAMGR, VCC, UBD]
bin_map = {STORMGR:"StorMgr", DATAMGR:"DataMgr", STORHVI:"ubd",VCC:"DataMgr",UBD:"ubd"}
dir_map = {STORMGR:"stor_mgr_ice", DATAMGR:"data_mgr", STORHVI:"stor_hvisor_ice", VCC:"data_mgr", UBD:"fds_client"}
udir_map = {STORMGR:"stor_mgr_ice", DATAMGR:"data_mgr", STORHVI:"fds_client", VCC:"stor_hvisor_ice",UBD:"fds_client"}
ut_map = {STORMGR:"sm_unit_test", DATAMGR:"dm_unit_test", STORHVI:"hvisor_uspace_test", VCC:"vcc_unit_test", UBD:"ubd"}
port_map = {STORMGR:6901, DATAMGR:6900, VCC:11000, UBD:10000}

#
# Defaults
#
cwd = "./"
prefix_base = "desktop_ut_"

#
# Relative to source/test dir
#
ice_home = "../lib/Ice-3.5.0"
ld_path = "../lib/:../lib/Ice-3.5.0/cpp/lib/:../lib/leveldb-1.12.0/:/usr/local/lib"

class TestSequenceFunctions(unittest.TestCase):

    def cleanupFiles(self, comp):
        #
        # Descend in the component's directory
        #
        pwd = os.getcwd()
        comp_dir = dir_map[comp]
        os.chdir(comp_dir)
        
        #
        # Remove all local files with this prefix
        #
        dentries  = os.listdir(cwd)
        for dentry in dentries:
            if re.match(prefix_base, dentry) != None:
                if os.path.isdir(dentry):
                    try:
                        shutil.rmtree("%s" % (dentry), ignore_errors=True)
                    except OSError as err:
                        print "Unable to remove stray dir: %s" % err
                else:
                    try:
                        os.remove("%s" % (dentry))
                    except OSError as err:
                        print "Unable to remove stray file: %s" % err

        #
        # Move back to previous directory
        #
        os.chdir(pwd)

    def tearDown(self):
        #
        # Kill any stray component processes
        #
        for comp in components:
            comp_bin = bin_map[comp]
            for proc in psutil.process_iter():
                if proc.name == comp:
                    proc.kill()

        #
        # Cleanup and stray files.
        # All files should start with
        # the prefix base.
        #
        for comp in components:
            self.cleanupFiles(comp)
                    
        print "Teardown complete."

    def setUp(self):
        print "Doing Setup in %s" % (cwd)
        #
        # Change to correct dir to run test
        #

        #
        # Clean up env so that it's ready for the next test
        #
        os.environ['ICE_HOME'] = ice_home
        os.environ['LD_LIBRARY_PATH'] = ld_path

        print "Setup complete"

    def start_server(self, server, ident):
        status = 0
        comp_bin = bin_map[server]
        comp_dir = dir_map[server]
        comp_port = port_map[server]

        #
        # Descend in the component's directory.
        #
        pwd = os.getcwd()
        os.chdir(comp_dir)

        #
        # Construct server command
        #
        comp_exe = cwd + comp_bin
        port_arg = "--port=%d" % (comp_port )
        prefix_arg = "--prefix=%s" % ("%s%d_" % (prefix_base, ident))
        comp_arg = port_arg + " " + prefix_arg
        cmd = comp_exe + " " + comp_arg
        print "Starting server cmd %s" % (cmd)

        #
        # Run server command in background
        #
        try :
            proc = subprocess.Popen(cmd, shell=True, stderr=subprocess.PIPE)
        except OSError as err:
            print "Failed to execute server command %s" (cmd)
            return None
        # Wait for server to come up
        if re.search("Waiting",cmd):
	    print " " 
        else:
            self.check_server(proc)

        #
        # Move back to previous directory
        #
        os.chdir(pwd)

        return proc

    def check_server(self, proc):
        while True:
            err_line = proc.stderr.readline()
            if re.search("accepting tcp connections", err_line):
                print "Server started as process %d" % (proc.pid)
                break

    def stop_server(self, proc):
        proc.terminate()

    def start_ut(self, server, ident, args=None):
        status = 0

        #
        # Descend in the component's directory
        #
        pwd = os.getcwd()
        comp_dir = udir_map[server]
        os.chdir(comp_dir)

        #
        # Construct unit test command
        #
        comp_ut = ut_map[server]
        comp_exe = cwd + comp_ut
        # Use the caller defined args if given or make our own
        if args == None:
            comp_port = port_map[server]
            port_arg = "--port=%d" % (comp_port )
            comp_arg = port_arg
        else:
            comp_arg = args
        comp_cmd = comp_exe + " " + comp_arg
        print "Starting unit test cmd %s" % (comp_cmd)

        #
        # Run unit test command synchrnously
        #
        try:     
            ut = subprocess.Popen(comp_cmd, shell=True, stdin=None, stdout=subprocess.PIPE)
            (stdoutdata, stderrdata) = ut.communicate(input=None)
            if ut.returncode != 0:
                print "Unit test failed to exit cleanly"
                status = 1
            if re.search("Unit test PASSED", stdoutdata):
                print "Unit test passed"
            else:
                print "Unit test failed"
                status = 1
        except OSError as err:
            print "Failed to  execute command %s" (comp_cmd)
            status = 1

        #
        # Move back to previous directory
        #
        os.chdir(pwd)

        return status

    def run_comp_test(self, comp, num_instances):
        status = 0
        instances = []

        #
        # Start server background instance
        #
        for i in range(0, num_instances):
            serv = self.start_server(comp, i)
            instances.append(serv)
        #
        # Run the unit test
        #
        for i in range(0, num_instances):
            status = self.start_ut(comp, i)
            if status != 0:
                print "Unit test instance %d FAILED" % (i)
                break

        # Kill the server
        for i in range(0, num_instances):
            serv = instances[i]
            self.stop_server(serv)

        return status
    
    @unittest.skipIf(len(sys.argv) > 1 and sys.argv[1] == "--jenkins",
                     "not supported when running on through jenkins")
    def test_ubd(self):
        test_name = "End-to-End UBD "
        status = 0
        num_instances = 1
        print "********** Starting test: %s **********" % (test_name)
        ut = subprocess.Popen("lsmod | grep fbd", stdout=subprocess.PIPE, shell=True)
        (stdoutdata, stderrdata) = ut.communicate(input=None)
        if re.search("fbd", stdoutdata):
        	print "********** fbd already loaded **********" 
        else:
        	subprocess.Popen("insmod stor_hvisor/fbd.ko", stdout=subprocess.PIPE, shell=True)
        	subprocess.Popen("stor_hvisor/tool/fds-cfg -d 299999", stdout=subprocess.PIPE, shell=True)
        #
        # Start SM
        #
        sm_serv = self.start_server(STORMGR, num_instances)
        #
        # Start DM
        #
        dm_serv = self.start_server(DATAMGR, num_instances)
        #
        #  ubd - run ubd server
        #
	ubd_serv = self.start_server(UBD, num_instances)
	#
        subprocess.Popen("fds_client/ubd_test", stdout=subprocess.PIPE, shell=True)
        #
        # Kill the SM server
        #
        self.stop_server(sm_serv)
        self.stop_server(dm_serv)
        self.stop_server(ubd_serv)

        self.assertEqual(status, 0)

        print "********** Stopping test: %s **********" % (test_name)

if __name__ == '__main__':
    #parser = optparse.OptionParser("usage: %prog [options]")
    #parser.add_option("-P", "--path", dest="path",
    #                  default="./", type="string",
    #                  help="path to source tree")
    #(options, args) = parser.parse_args()
    #path = options.path
    #print "Using path %s" % (path)

    #unittest.main()
    if len(sys.argv) > 1 and sys.argv[1] == "--jenkins":
        print "running in jenkins mode"
        import junitxml
        ld_path = "../libs"
        fp = file('results.xml', 'wb')
        result = junitxml.JUnitXmlResult(fp)
        suite = unittest.TestLoader().loadTestsFromTestCase(TestSequenceFunctions)
        unittest.TestSuite(suite).run(result)
        result.stopTestRun()
    else:
        suite = unittest.TestLoader().loadTestsFromTestCase(TestSequenceFunctions)
        unittest.TextTestRunner(verbosity=2).run(suite)
