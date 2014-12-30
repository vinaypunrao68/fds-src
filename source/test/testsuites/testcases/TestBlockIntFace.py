#!/usr/bin/python
#
# Copyright 2014 by Formation Data Systems, Inc.
#

# FDS test-case pattern requirements.
import unittest
import traceback
import TestCase

# Module-specific requirements
import sys
import os
import subprocess
import time

nbd_device = "/dev/nbd15"
pwd = ""

# This class contains the attributes and methods to test
# the FDS interface to create a block volume.
#
class TestBlockCrtVolume(TestCase.FDSTestCase):
    def __init__(self, parameters=None):
        super(TestBlockCrtVolume, self).__init__(parameters)


    def runTest(self):
        test_passed = True

        if TestCase.pyUnitTCFailure:
            self.log.warning("Skipping Case %s. stop-on-fail/failfast set and a previous test case has failed." %
                             self.__class__.__name__)
            return unittest.skip("stop-on-fail/failfast set and a previous test case has failed.")
        else:
            self.log.info("Running Case %s." % self.__class__.__name__)

        try:
            if not self.test_BlockCrtVolume():
                test_passed = False
        except Exception as inst:
            self.log.error("Creating an block volume caused exception:")
            self.log.error(traceback.format_exc())
            self.log.error(inst.message)
            test_passed = False

        super(self.__class__, self).reportTestCaseResult(test_passed)

        # If there is any test fixture teardown to be done, do it here.

        if self.parameters["pyUnit"]:
            self.assertTrue(test_passed)
        else:
            return test_passed


    def test_BlockCrtVolume(self):
        """
        Test Case:
        Attempt to create a block volume.
        """

        # TODO(Andrew): We should call OM's REST endpoint with an auth
        # token. Using fdscli is just me being lazy...
        fdscfg = self.parameters["fdscfg"]
        nodes = fdscfg.rt_obj.cfg_nodes
        fds_root = nodes[0].nd_conf_dict['fds_root']
        bin_dir = fdscfg.rt_env.get_bin_dir(debug=False)
        global pwd
        pwd = os.getcwd()
        os.chdir(bin_dir)

        # Block volume create command
        # TODO(Andrew): Don't hard code volume name
        blkCrtCmd = "./fdscli --fds-root=" + fds_root + " --volume-create blockVolume -i 1 -s 10240 -p 50 -y blk"
        result = subprocess.call(blkCrtCmd, shell=True)
        if result != 0:
            self.log.error("Failed to create block volume")
            return False
        time.sleep(5)

        blkModCmd = "./fdscli --fds-root=" + fds_root + " --volume-modify \"blockVolume\" -s 10240 -g 0 -m 0 -r 10"
        result = subprocess.call(blkModCmd, shell=True)
        if result != 0:
            self.log.error("Failed to modify block volume")
            return False
        time.sleep(5)

        return True

# This class contains the attributes and methods to test
# the FDS interface to attach a volume.
#
class TestBlockAttachVolume(TestCase.FDSTestCase):
    def __init__(self, parameters=None):
        super(TestBlockAttachVolume, self).__init__(parameters)


    def runTest(self):
        test_passed = True

        if TestCase.pyUnitTCFailure:
            self.log.warning("Skipping Case %s. stop-on-fail/failfast set and a previous test case has failed." %
                             self.__class__.__name__)
            return unittest.skip("stop-on-fail/failfast set and a previous test case has failed.")
        else:
            self.log.info("Running Case %s." % self.__class__.__name__)

        try:
            if not self.test_BlockAttVolume():
                test_passed = False
        except Exception as inst:
            self.log.error("Attaching a block volume caused exception:")
            self.log.error(traceback.format_exc())
            self.log.error(inst.message)
            test_passed = False

        super(self.__class__, self).reportTestCaseResult(test_passed)

        # If there is any test fixture teardown to be done, do it here.

        if self.parameters["pyUnit"]:
            self.assertTrue(test_passed)
        else:
            return test_passed


    def test_BlockAttVolume(self):
        """
        Test Case:
        Attempt to attach a block volume.
        """

        # TODO(Andrew): We shouldn't hard code the path, volume name, port, ip
        blkAttCmd = ['sudo', '../../../cinder/nbdadm.py', 'attach', 'localhost', 'blockVolume']
        nbdadm = subprocess.Popen(blkAttCmd, stderr=subprocess.PIPE, stdout=subprocess.PIPE)
        (stdout, stderr) = nbdadm.communicate()
        if nbdadm.returncode != 0:
            self.log.error("Failed to attach block volume %s %s" % (stdout, stderr))
            return False
        else:
            global nbd_device
            nbd_device = stdout.rstrip()
        self.log.info("Attached block device %s" % (nbd_device))
        time.sleep(5)

        return True

# This class contains the attributes and methods to test
# the FDS interface to disconnect a volume.
#
class TestBlockDetachVolume(TestCase.FDSTestCase):
    def __init__(self, parameters=None):
        super(TestBlockDetachVolume, self).__init__(parameters)

    def runTest(self):
        test_passed = True

        if TestCase.pyUnitTCFailure:
            self.log.warning("Skipping Case %s. stop-on-fail/failfast set and a previous test case has failed." %
                             self.__class__.__name__)
            return unittest.skip("stop-on-fail/failfast set and a previous test case has failed.")
        else:
            self.log.info("Running Case %s." % self.__class__.__name__)

        try:
            if not self.test_BlockDetachVolume():
                test_passed = False
        except Exception as inst:
            self.log.error("Disconnecting a block volume caused exception:")
            self.log.error(traceback.format_exc())
            self.log.error(inst.message)
            test_passed = False

        super(self.__class__, self).reportTestCaseResult(test_passed)

        # If there is any test fixture teardown to be done, do it here.

        if self.parameters["pyUnit"]:
            self.assertTrue(test_passed)
        else:
            return test_passed


    def test_BlockDetachVolume(self):
        """
        Test Case:
        Attempt to detach a block volume.
        """

        # Command to detach a block volume
        # TODO(Andrew): Don't hard code path, volume name
        blkDetachCmd = "sudo ../../../cinder/nbdadm.py detach blockVolume"
        result = subprocess.call(blkDetachCmd, shell=True)
        if result != 0:
            self.log.error("Failed to detach block volume")
            return False
        time.sleep(5)

        os.chdir(pwd)
        return True

# This class contains the attributes and methods to test
# writing block data
#
class TestBlockFioW(TestCase.FDSTestCase):
    def __init__(self, parameters=None):
        super(TestBlockFioW, self).__init__(parameters)


    def runTest(self):
        test_passed = True

        if TestCase.pyUnitTCFailure:
            self.log.warning("Skipping Case %s. stop-on-fail/failfast set and a previous test case has failed." %
                             self.__class__.__name__)
            return unittest.skip("stop-on-fail/failfast set and a previous test case has failed.")
        else:
            self.log.info("Running Case %s." % self.__class__.__name__)

        try:
            if not self.test_BlockFioWrite():
                test_passed = False
        except Exception as inst:
            self.log.error("Writing a block volume caused exception:")
            self.log.error(traceback.format_exc())
            self.log.error(inst.message)
            test_passed = False

        super(self.__class__, self).reportTestCaseResult(test_passed)

        # If there is any test fixture teardown to be done, do it here.

        if self.parameters["pyUnit"]:
            self.assertTrue(test_passed)
        else:
            return test_passed


    def test_BlockFioWrite(self):
        """
        Test Case:
        Attempt to write to a block volume.
        """

        # TODO(Andrew): Don't hard code all of this stuff...
        fioCmd = "sudo fio --name=seq-writers --readwrite=write --ioengine=libaio --direct=1 --bs=4k --iodepth=128 --numjobs=4 --size=10485760 --filename=%s --verify=md5 --verify_fatal=1" % (nbd_device)
        result = subprocess.call(fioCmd, shell=True)
        if result != 0:
            self.log.error("Failed to run write workload")
            return False
        time.sleep(5)

        return True

# This class contains the attributes and methods to test
# reading/writing block data
#
class TestBlockFioRW(TestCase.FDSTestCase):
    def __init__(self, parameters=None):
        super(TestBlockFioRW, self).__init__(parameters)


    def runTest(self):
        test_passed = True

        if TestCase.pyUnitTCFailure:
            self.log.warning("Skipping Case %s. stop-on-fail/failfast set and a previous test case has failed." %
                             self.__class__.__name__)
            return unittest.skip("stop-on-fail/failfast set and a previous test case has failed.")
        else:
            self.log.info("Running Case %s." % self.__class__.__name__)

        try:
            if not self.test_BlockFioReadWrite():
                test_passed = False
        except Exception as inst:
            self.log.error("Reading/writing a block volume caused exception:")
            self.log.error(traceback.format_exc())
            self.log.error(inst.message)
            test_passed = False

        super(self.__class__, self).reportTestCaseResult(test_passed)

        # If there is any test fixture teardown to be done, do it here.

        if self.parameters["pyUnit"]:
            self.assertTrue(test_passed)
        else:
            return test_passed


    def test_BlockFioReadWrite(self):
        """
        Test Case:
        Attempt to read/write to a block volume.
        """

        # TODO(Andrew): Don't hard code all of this stuff...
        fioCmd = "sudo fio --name=rand-readers --readwrite=readwrite --ioengine=libaio --direct=1 --bs=4k --iodepth=128 --numjobs=4 --size=10485760 --filename=%s" % (nbd_device)
        result = subprocess.call(fioCmd, shell=True)
        if result != 0:
            self.log.error("Failed to run read/write workload")
            return False
        time.sleep(5)

        return True

if __name__ == '__main__':
    TestCase.FDSTestCase.fdsGetCmdLineConfigs(sys.argv)
    unittest.main()
