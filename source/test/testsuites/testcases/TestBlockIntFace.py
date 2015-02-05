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
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_BlockCrtVolume,
                                             "Creating an block volume")

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
        cur_dir = os.getcwd()
        os.chdir(bin_dir)

        # Block volume create command
        # TODO(Andrew): Don't hard code volume name
        blkCrtCmd = "./fdscli --fds-root=" + fds_root + " --volume-create blockVolume -i 1 -s 10240 -p 50 -y blk"
        result = subprocess.call(blkCrtCmd, shell=True)
        if result != 0:
            os.chdir(cur_dir)
            self.log.error("Failed to create block volume")
            return False
        time.sleep(5)

        blkModCmd = "./fdscli --fds-root=" + fds_root + " --volume-modify \"blockVolume\" -s 10240 -g 0 -m 0 -r 10"
        result = subprocess.call(blkModCmd, shell=True)
        if result != 0:
            os.chdir(cur_dir)
            self.log.error("Failed to modify block volume")
            return False

        os.chdir(cur_dir)

        time.sleep(5)

        return True

# This class contains the attributes and methods to test
# the FDS interface to attach a volume.
#
class TestBlockAttachVolume(TestCase.FDSTestCase):
    def __init__(self, parameters=None, volume=None):
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_BlockAttVolume,
                                             "Attaching a block volume")

        self.passedVolume = volume

    def test_BlockAttVolume(self):
        """
        Test Case:
        Attempt to attach a block volume client.
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]
        om_node = fdscfg.rt_om_node

        volume = 'blockVolume'

        # Check if a volume was passed to us.
        if self.passedVolume is not None:
            volume = self.passedVolume

        cinder_dir = os.path.join(fdscfg.rt_env.get_fds_source(), 'cinder')
        blkAttCmd = '%s/nbdadm.py attach %s %s' % (cinder_dir, om_node.nd_conf_dict['ip'], volume)

        # Parameter return_stdin is set to return stdout. ... Don't ask me!
        status, stdout = om_node.nd_agent.exec_wait(blkAttCmd, return_stdin=True)

        if status != 0:
            self.log.error("Failed to attach block volume %s: %s." % (volume, stdout))
            return False
        else:
            global nbd_device
            nbd_device = stdout.rstrip()

        time.sleep(5)
        self.log.info("Attached block device %s" % (nbd_device))

        return True

# This class contains the attributes and methods to test
# the FDS interface to disconnect a volume.
#
class TestBlockDetachVolume(TestCase.FDSTestCase):
    def __init__(self, parameters=None, volume=None):
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_BlockDetachVolume,
                                             "Disconnecting a block volume")

        self.passedVolume = volume

    def test_BlockDetachVolume(self):
        """
        Test Case:
        Attempt to detach a block volume.
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]
        om_node = fdscfg.rt_om_node

        volume = 'blockVolume'

        # Check if a volume was passed to us.
        if self.passedVolume is not None:
            volume = self.passedVolume

        cinder_dir = os.path.join(fdscfg.rt_env.get_fds_source(), 'cinder')

        # Command to detach a block volume
        blkDetachCmd = '%s/nbdadm.py detach %s' % (cinder_dir, volume)

        status = om_node.nd_agent.exec_wait(blkDetachCmd)

        if status != 0:
            self.log.error("Failed to detach block volume with status %s." % status)
            return False

        time.sleep(5)

        return True

# This class contains the attributes and methods to test
# writing block data
#
class TestBlockFioSeqW(TestCase.FDSTestCase):
    def __init__(self, parameters=None, waitComplete="TRUE"):
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_BlockFioWrite,
                                             "Writing a block volume")

        if waitComplete == "TRUE":
            self.passedWaitComplete = True
        else:
            self.passedWaitComplete = False

    def test_BlockFioWrite(self):
        """
        Test Case:
        Attempt to write to a block volume.
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]
        om_node = fdscfg.rt_om_node

        # TODO(Andrew): Don't hard code all of this stuff...
        fioCmd = "sudo fio --name=seq-writers --readwrite=write --ioengine=libaio --direct=1 --bsrange=512-128k --iodepth=128 --numjobs=1 --size=16m --filename=%s --verify=md5 --verify_fatal=1" % (nbd_device)

        status = om_node.nd_agent.exec_wait(fioCmd, wait_compl=self.passedWaitComplete)

        if status != 0:
            self.log.error("Failed to run write workload with status %s." % status)
            return False

        return True

# This class contains the attributes and methods to test
# writing block data randomly
#
class TestBlockFioRandW(TestCase.FDSTestCase):
    def __init__(self, parameters=None):
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_BlockFioRandW,
                                             "Randomly writing a block volume")

    def test_BlockFioRandW(self):
        """
        Test Case:
        Attempt to write to a block volume.
        """

        # TODO(Andrew): Don't hard code all of this stuff...
        fioCmd = "sudo fio --name=rand-writers --readwrite=randwrite --ioengine=libaio --direct=1 --bsrange=512-128k --iodepth=128 --numjobs=1 --size=16m --filename=%s --verify=md5 --verify_fatal=1" % (nbd_device)
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
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_BlockFioReadWrite,
                                             "Reading/writing a block volume")

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
