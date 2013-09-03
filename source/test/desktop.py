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
import junitxml

#
# Components
# Uses hackey enums for components
# 
INVALID = 0
STORMGR = 1
DATAMGR = 2
STORHVI = 3
VCC = 4
components = [STORMGR, DATAMGR, VCC]
bin_map = {STORMGR:"StorMgr", DATAMGR:"DataMgr", VCC:"DataMgr"}
dir_map = {STORMGR:"stor_mgr_ice", DATAMGR:"data_mgr", STORHVI:"stor_hvisor_ice", VCC:"data_mgr"}
udir_map = {STORMGR:"stor_mgr_ice", DATAMGR:"data_mgr", STORHVI:"fds_client", VCC:"stor_hvisor_ice"}
ut_map = {STORMGR:"sm_unit_test", DATAMGR:"dm_unit_test", STORHVI:"hvisor_uspace_test", VCC:"vcc_unit_test"}
port_map = {STORMGR:10000, DATAMGR:11000, VCC:11000}

#
# Defaults
#
cwd = "./"
prefix_base = "desktop_ut_"

#
# Relative to source/test dir
#
ice_home = "../lib/Ice-3.5.0"
ld_path = "../libs"

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
        port_arg = "--port=%d" % (comp_port + ident)
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
            port_arg = "--port=%d" % (comp_port + ident)
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

#    def test_stormgr(self):
#        test_name = "Storage Manager"
#        num_instances = 5
#        print "********** Starting test: %s **********" % (test_name)
#
#        status = 0
#        status = self.run_comp_test(STORMGR, num_instances)
#        self.assertEqual(status, 0)
#
#        print "********** Stopping test: %s **********" % (test_name)

    def test_datamgr(self):
        test_name = "Data Manager"
        num_instances = 5
        print "********** Starting test: %s **********" % (test_name)
            
        status = self.run_comp_test(DATAMGR, num_instances)
        self.assertEqual(status, 0)
            
        print "********** Stopping test: %s **********" % (test_name)

    def test_vcc(self):
        test_name = "Volume catalog cache"
        num_instances = 5
        print "********** Starting test: %s **********" % (test_name)

        status = self.run_comp_test(VCC, num_instances)
        self.assertEqual(status, 0)

        print "********** Stopping test: %s **********" % (test_name)

    def test_sh(self):
        test_name = "Storage Hypervisor"
        status = 0
        num_instances = 1
        print "********** Starting test: %s **********" % (test_name)

        #
        # Start SM
        #
        sm_serv = self.start_server(STORMGR, num_instances)

        #
        # Start DM
        #
        dm_serv = self.start_server(DATAMGR, num_instances)

        #
        # Start SH unit test
        #
        sm_port = port_map[STORMGR] + num_instances
        dm_port = port_map[DATAMGR] + num_instances
        args = " --unit_test --sm_port=%d --dm_port=%d" % (sm_port, dm_port)
        status = self.start_ut(STORHVI, num_instances, args)

        #
        # Kill the SM server
        #
        self.stop_server(sm_serv)
        self.stop_server(dm_serv)

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
    fp = file('results.xml', 'wb')
    result = junitxml.JUnitXmlResult(fp)
    result.startTestRun()
    suite = unittest.TestLoader().loadTestsFromTestCase(TestSequenceFunctions)
    unittest.TestSuite(suite).run(result)
    result.stopTestRun()
#    unittest.TextTestRunner(verbosity=2).run(suite)
