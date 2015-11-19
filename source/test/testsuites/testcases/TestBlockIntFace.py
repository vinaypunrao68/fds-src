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
from fdscli.model.volume.settings.block_settings import BlockSettings
from fdscli.model.common.size import Size
from fdscli.model.volume.volume import Volume
from fdslib.TestUtils import get_volume_service
from fdscli.model.fds_error import FdsError

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
        # token.
        fdscfg = self.parameters["fdscfg"]
        om_node = fdscfg.rt_om_node
        global pwd

        # Block volume create command
        # TODO(Andrew): Don't hard code volume name
        new_volume = Volume();
        new_volume.name= 'nbd_vol'
        access = BlockSettings()
        access.capacity = Size( size = '104857600', unit = 'B')
        new_volume.settings = access
        vol_service = get_volume_service(self,om_node.nd_conf_dict['ip'])
        status = vol_service.create_volume(new_volume)
        if isinstance(status, FdsError):
            self.log.error("Volume nbd+vol creation on %s returned status %s." %
                               (om_node.nd_conf_dict['node-name'], status))
            return False

        return True

# This class contains the attributes and methods to test
# the FDS interface to list the exports.
#
class TestBlockListVolumes(TestCase.FDSTestCase):
    def __init__(self, parameters=None):
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_ListVolumes,
                                             "List nbd-client volumes")

    def test_ListVolumes(self):
        """
        Test Case:
        Attempt to cause a list volumes
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]
        om_node = fdscfg.rt_om_node

        blkListExportCmd = 'nbd-client -l %s' % (om_node.nd_conf_dict['ip'] )

        # Parameter return_stdin is set to return stdout. ... Don't ask me!
        status, stdout = om_node.nd_agent.exec_wait(blkListExportCmd, return_stdin=True)

        if status != 0:
            self.log.info("Failed to list block volumes as expected.")
            return True

        self.log.error("Error, volumes returned: %s." % (stdout))
        return False

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

        volume = 'nbd_vol'

	#Per FS-1368, make use of --lockfile /tmp/nbdadm/nbdadm.lock
	nbdadm_tmp = '/tmp/nbdadm'
	nbdadm_lockfile = 'nbdadm.lock'

	if not os.path.exists(nbdadm_tmp):
		os.makedirs('%s' % nbdadm_tmp)

        # Check if a volume was passed to us.
        if self.passedVolume is not None:
            volume = self.passedVolume

        cinder_dir = os.path.join(fdscfg.rt_env.get_fds_source(), 'cinder')
        #blkAttCmd = '%s/nbdadm.py attach %s %s' % (cinder_dir, om_node.nd_conf_dict['ip'], volume)
        blkAttCmd = '%s/nbdadm.py --lockfile %s/%s attach %s %s' % (cinder_dir, nbdadm_tmp, nbdadm_lockfile, om_node.nd_conf_dict['ip'], volume)

        # Parameter return_stdin is set to return stdout. ... Don't ask me!
        status, stdout = om_node.nd_agent.exec_wait(blkAttCmd, return_stdin=True)

        if status != 0:
            self.log.error("Failed to attach block volume %s: %s." % (volume, stdout))
            return False
        else:
            global nbd_device
            nbd_device = stdout.rstrip()

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

        return True

# This class contains the attributes and methods to test
# writing block data.
#
class TestBlockFioSeqW(TestCase.FDSTestCase):
    def __init__(self, parameters=None, volume=None, expect_failure=False):
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_BlockFioWrite,
                                             "Writing a block volume")
        self.passedVol = volume
        self.expectFailure = expect_failure

    def test_BlockFioWrite(self):
        """
        Test Case:
        Attempt to write to a block volume.
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]
        om_node = fdscfg.rt_om_node

        if self.childPID is None:
            # Not running in a forked process.
            # Stop on failures.
            verify_fatal = 1
        else:
            # Running in a forked process. Don't
            # stop on failures.
            verify_fatal = 0

        if self.passedVol is not None:
            volumes = fdscfg.rt_get_obj('cfg_volumes')
            for volume in volumes:
                if self.passedVol == volume.nd_conf_dict['vol-name']:
                    global nbd_device
                    nbd_device = volume.nd_conf_dict['nbd-dev']
                    self.log.info("nbd_device is %s" % (nbd_device))
                    break

        # TODO(Andrew): Don't hard code all of this stuff...
        fioCmd = "sudo fio --name=seq-writers --readwrite=write --ioengine=posixaio --direct=1 --bsrange=512-1M " \
                 "--iodepth=128 --numjobs=1 --fill_device=1 --filename=%s --verify=md5 --verify_fatal=%d" %\
                 (nbd_device, verify_fatal)
        status = om_node.nd_agent.exec_wait(fioCmd)

        if self.expectFailure:
            if status == 0:
                self.log.error("Expected to fail to run write workload with status.")
                return False
        elif status != 0:
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

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]
        om_node = fdscfg.rt_om_node

        # TODO(Andrew): Don't hard code all of this stuff...
        fioCmd = "sudo fio --name=rand-writers --readwrite=randwrite --ioengine=posixaio --direct=1 --bsrange=512-1M --iodepth=128 --numjobs=1 --fill_device=1 --filename=%s --verify=md5 --verify_fatal=1" % (nbd_device)
        status = om_node.nd_agent.exec_wait(fioCmd)
        if status != 0:
            self.log.error("Failed to run write workload with status %s." % status)
            return False

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

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]
        om_node = fdscfg.rt_om_node

        # TODO(Andrew): Don't hard code all of this stuff...
        fioCmd = "sudo fio --name=rw --readwrite=readwrite --ioengine=posixaio --direct=1 --bsrange=512-1M --iodepth=128 --numjobs=1 --size=50M --filename=%s" % (nbd_device)
        status = om_node.nd_agent.exec_wait(fioCmd)
        if status != 0:
            self.log.error("Failed to run read/write workload with status %s." % status)
            return False

        return True

# This class contains the attributes and methods to test
# reading/writing random block data
#
class TestBlockFioRandRW(TestCase.FDSTestCase):
    def __init__(self, parameters=None):
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_BlockFioReadWrite,
                                             "Reading/writing a block volume")

    def test_BlockFioReadWrite(self):
        """
        Test Case:
        Attempt to random read/write to a block volume.
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]
        om_node = fdscfg.rt_om_node

        # TODO(Andrew): Don't hard code all of this stuff...
        fioCmd = "sudo fio --name=rand-rw --readwrite=randrw --ioengine=posixaio --direct=1 --bs=512-1M --iodepth=128 --numjobs=1 --size=50M --filename=%s" % (nbd_device)
        status = om_node.nd_agent.exec_wait(fioCmd)
        if status != 0:
            self.log.error("Failed to run random read/write workload with status %s." % status)
            return False

        return True

if __name__ == '__main__':
    TestCase.FDSTestCase.fdsGetCmdLineConfigs(sys.argv)
    unittest.main()
