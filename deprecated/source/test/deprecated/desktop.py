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
import hashlib
import shlex

#
# Defaults
#
cwd = "./"
prefix_base = "desktop_ut_"

#
# Components
# Uses hackey enums for components
# 
INVALID = 0
STORMGR = 1
DATAMGR = 2
STORHVI = 3
VCC = 4
OM = 5

#
# The desktop test must run from FDS src root dir
# and must be run on the default build directory (usually 'release')
#
proc   = subprocess.Popen(shlex.split('uname -s -m'), stdout=subprocess.PIPE)
os_dir = proc.communicate()[0].lower().rstrip('\n').replace(' ', '-')

fds_root_dir = os.path.abspath('.')
fds_bin_dir  = os.path.join(fds_root_dir, 'Build', os_dir, 'bin')
fds_test_dir = os.path.join(fds_root_dir, 'Build', os_dir, 'tests')
fds_dump_dir = os.path.join(fds_root_dir, 'Build/TestData')

#
# Get absolute path for lib because we'll execute inside bin/test dir, not
# from the FDS source root dir.
#
ice_home = os.path.abspath(os.path.join(fds_root_dir, "../Ice-3.5.0"))
ld_path  = (
    os.path.abspath(os.path.join(fds_root_dir, '../Ice-3.5.0/cpp/lib/')) + ':' +
    os.path.abspath(os.path.join(fds_root_dir, '../leveldb-1.12.0/')) + ':' +
    os.path.abspath(os.path.join(fds_root_dir, 'libs/')) + ':' +
    '/usr/local/lib'
)

components = [STORMGR, DATAMGR, VCC, OM]
bin_map = {
    STORMGR : "StorMgr",
    DATAMGR : "DataMgr",
    VCC     : "DataMgr",
    OM      : "orchMgr"
}
bin_args = {
    OM      : "--test",
    STORMGR : "--test_mode",
    DATAMGR : "--test_mode",
    VCC     : "--test_mode"
}
use_fds_root_set = {
    STORMGR,
    DATAMGR,
    STORHVI,
    VCC
}
dir_map = {
    STORMGR : fds_bin_dir,
    DATAMGR : fds_bin_dir,
    STORHVI : fds_bin_dir,
    VCC     : fds_bin_dir,
    OM      : fds_bin_dir
}
udir_map = {
    STORMGR : fds_test_dir,
    DATAMGR : fds_test_dir,
    STORHVI : fds_test_dir,
    VCC     : fds_test_dir,
    OM      : fds_test_dir
}
ut_map = {
    STORMGR  : "sm_unit_test",
    DATAMGR  : "dm_unit_test",
    STORHVI  : "hvisor_uspace_test",
    VCC      : "vcc_unit_test",
    OM       : "om_unit_test"
}
port_map = {
    STORMGR  : 10000,
    DATAMGR  : 11000,
    VCC      : 11000,
    OM       : 14000
}
cp_port_map = {
    STORMGR  : 13000,
    DATAMGR  : 12000,
    VCC      : 12000
}

class TestSequenceFunctions(unittest.TestCase):

    def cleanupFiles(self, comp=None):
        return 0

        #
        # Descend in the component's directory
        #
        pwd = os.getcwd()
        if comp == None:
            comp_dir = cwd
        else:
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
        # Remove all files under FDS specific storage
        # using new presistent layer storage
        #
        absPath = "/" + prefix_base
        shutil.rmtree(absPath, ignore_errors=True)
        # TODO: Remove this line once presistance layer
        # uses relative paths
        shutil.rmtree("/fds", ignore_errors=True)

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
                if proc.name == comp_bin:
                    proc.kill()

        #
        # Cleanup and stray files.
        # All files should start with
        # the prefix base.
        #
        self.cleanupFiles()
        for comp in components:
            self.cleanupFiles(comp)

        #
        # Clean up fds root
        #
        os.system('rm -rf ' + fds_dump_dir)
                    
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
        # Make the server's directory
        #
        use_root = server in use_fds_root_set
        if use_root:
            os.system('mkdir -p ' + fds_dump_dir + "/" + prefix_base + str(ident) + "/hdd")
            os.system('mkdir -p ' + fds_dump_dir + "/" + prefix_base + str(ident) + "/ssd")

        cp_port = None
        if server in cp_port_map:
            cp_port = cp_port_map[server]

        extra_args = None
        if server in bin_args:
            extra_args = bin_args[server]

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
        if cp_port != None:
            cp_port_arg = " --cp_port=%d" % (cp_port + ident)
            prefix_arg += cp_port_arg
        root_arg = ""
        if use_root:
            root_arg = "--fds-root=%s/%s%d" % (fds_dump_dir, prefix_base, ident)
        if extra_args != None:
            prefix_arg += " " + extra_args
        comp_arg = port_arg + " " + prefix_arg + " " + root_arg
        cmd = comp_exe + " " + comp_arg
        cmd = "ulimit -s 4096; ulimit -c unlimited; %s" % (cmd)
        print "Starting server cmd %s" % (cmd)

        #
        # Run server command in background
        #
        try :
            proc = subprocess.Popen(cmd,
                                    shell=True,
                                    stderr=subprocess.PIPE)
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
        # Make the root directory
        #
        use_root = server in use_fds_root_set
        if use_root:
            os.system('mkdir -p ' + fds_dump_dir + "/" + prefix_base + str(ident) + "/hdd")
            os.system('mkdir -p ' + fds_dump_dir + "/" + prefix_base + str(ident) + "/ssd")

        #
        # Construct unit test command
        #
        comp_ut = ut_map[server]
        comp_exe = cwd + comp_ut
        # Use the caller defined args if given or make our own
        if args == None:
            comp_port = port_map[server]
            port_arg = "--port=%d" % (comp_port + ident)
            if server in cp_port_map:
                cp_port = cp_port_map[server]
                cp_port_arg = " --cp_port=%d" % (cp_port + ident)
                port_arg += cp_port_arg
            comp_arg = port_arg
            if server == VCC:
                comp_arg = comp_arg + " --fds-root=%s/%s%d" % (fds_dump_dir, prefix_base, ident)
        else:
            comp_arg = args
        comp_cmd = comp_exe + " " + comp_arg
        comp_cmd = "ulimit -s 4096; ulimit -c unlimited; %s" % (comp_cmd)
        print "Starting unit test cmd %s" % (comp_cmd)

        #
        # Run unit test command synchrnously
        #
        try:     
            ut = subprocess.Popen(comp_cmd,
                                  shell=True,
                                  stdin=None,
                                  stdout=subprocess.PIPE)
            (stdoutdata, stderrdata) = ut.communicate(input=None)
            if ut.returncode != 0:
                print "Unit test failed to exit cleanly, err code %d" % ut.returncode
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

    def test_helloworld(self):
        test_name = "Hello World"
        print "********** Starting test: %s **********" % (test_name)

        status = 0
        print "Hello World :-)"
        self.assertEqual(status, 0)
            
        print "********** Stopping test: %s **********" % (test_name)
    
    @unittest.skip("Skipping ICE version of Storage Manager test")
    def test_stormgr(self):
        test_name = "Storage Manager"
        num_instances = 5
        print "********** Starting test: %s **********" % (test_name)

        status = 0
        status = self.run_comp_test(STORMGR, num_instances)
        self.assertEqual(status, 0)

        print "********** Stopping test: %s **********" % (test_name)

    @unittest.skip("Skipping ICE version of Data Manager test")
    def test_datamgr(self):
        test_name = "Data Manager"
        num_instances = 5
        print "********** Starting test: %s **********" % (test_name)
            
        status = 0
        status = self.run_comp_test(DATAMGR, num_instances)
        self.assertEqual(status, 0)
            
        print "********** Stopping test: %s **********" % (test_name)

    @unittest.skip("Skipping ICE version of Volume Catalog Cache test")
    def test_vcc(self):
        test_name = "Volume catalog cache"
        num_instances = 5
        print "********** Starting test: %s **********" % (test_name)

        status = 0
        status = self.run_comp_test(VCC, num_instances)
        self.assertEqual(status, 0)

        print "********** Stopping test: %s **********" % (test_name)

    @unittest.skip("Skipping ICE version of Orchestration Manager test")
    def test_om(self):
        test_name = "Orchestration manager"
        num_instances = 5
        print "********** Starting test: %s **********" % (test_name)

        status = 0
        status = self.run_comp_test(OM, num_instances)
        self.assertEqual(status, 0)

        print "********** Stopping test: %s **********" % (test_name)

    @unittest.skip("Skipping ICE version of Access Manager test")
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
        args = " --unit_test --sm_port=%d --dm_port=%d --fds-root=%s/%s%d" % (sm_port,
                                                                             dm_port,
                                                                             fds_dump_dir,
                                                                             prefix_base,
                                                                             num_instances)
        print "Using", args
        status = self.start_ut(STORHVI, num_instances, args)

        #
        # Kill the SM server
        #
        self.stop_server(sm_serv)
        self.stop_server(dm_serv)

        self.assertEqual(status, 0)

        print "********** Stopping test: %s **********" % (test_name)

    @unittest.skip("Skipping ICE version of Cluster File Copy test")
    def test_file_cp(self):
        test_name = "File Copy"
        status = 0
        num_instances = 1
        org_file = os.path.join(dir_map[STORMGR], bin_map[STORMGR])
        tmp_file = os.path.join(fds_test_dir, prefix_base + "tmp_file.in")
        out_file = os.path.join(fds_test_dir, prefix_base + "tmp_file.out")
        print "********** Starting test: %s **********" % (test_name)

        #
        # Copy a local file to a temp file so that we don't
        # screw up the original copy then read the temp file.
        #
        shutil.copy2(org_file, tmp_file)
        fd = open(tmp_file, 'r')
        file_data = fd.read()
        fd.close()

        #
        # Calc tmp file's hash
        #
        sha = hashlib.sha1()
        sha.update(file_data)
        tmp_digest = sha.hexdigest()

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
        args = " --ut_file --infile=%s --outfile=%s --sm_port=%d --dm_port=%d --fds-root=%s/%s%d" % (tmp_file,
                                                                                                     out_file,
                                                                                                     sm_port,
                                                                                                     dm_port,
                                                                                                     fds_dump_dir,
                                                                                                     prefix_base,
                                                                                                     num_instances)
        status = self.start_ut(STORHVI, num_instances, args)        

        #
        # Kill the SM server
        #
        self.stop_server(sm_serv)
        self.stop_server(dm_serv)

        #
        # Read output file's contents
        #
        fd = open(out_file, 'r')
        out_data = fd.read()
        fd.close()

        #
        # Calc output file's hash
        #
        sha = hashlib.sha1()
        sha.update(out_data)
        out_digest = sha.hexdigest()
        print "Input file digest %s" % (tmp_digest)
        print "Output file digest %s" % (out_digest)

        if tmp_digest != out_digest:
            status = -1;

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
        fp = file('results.xml', 'wb')
        result = junitxml.JUnitXmlResult(fp)
        suite = unittest.TestLoader().loadTestsFromTestCase(TestSequenceFunctions)
        unittest.TestSuite(suite).run(result)
        result.stopTestRun()
    elif len(sys.argv) > 1 and sys.argv[1] != "--jenkins":
        print "running in single test mode"
        #We're creating a suite from the test provided at the command line
        suite = unittest.TestSuite()
        suite.addTest(TestSequenceFunctions(sys.argv[1]))
        unittest.TextTestRunner(verbosity=2).run(suite)
    else:
        suite = unittest.TestLoader().loadTestsFromTestCase(TestSequenceFunctions)
        unittest.TextTestRunner(verbosity=2).run(suite)

        # These just run the SM test
        #suite = unittest.TestSuite()
        #suite.addTest(TestSequenceFunctions('test_stormgr'))
        #unittest.TextTestRunner(verbosity=2).run(suite)
