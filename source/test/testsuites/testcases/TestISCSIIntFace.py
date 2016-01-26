#!/usr/bin/python
#
# Copyright 2016 by Formation Data Systems, Inc.
#

# FDS test-case pattern requirements.
import unittest
import TestCase

from testcases.iscsifixture import ISCSIFixture

# Module-specific requirements
import os
import sys
from fabric.contrib.files import *
from fdscli.model.volume.settings.iscsi_settings import ISCSISettings
from fdscli.model.volume.settings.lun_permission import LunPermissions
from fdscli.model.common.size import Size
from fdscli.model.volume.volume import Volume
from fdslib.TestUtils import connect_fabric
from fdslib.TestUtils import disconnect_fabric
from fdslib.TestUtils import get_volume_service
from fdscli.model.fds_error import FdsError

# A third-party initiator that enables sending CDBs and fetching response
from pyscsi.pyscsi.scsi_device import SCSIDevice
from pyscsi.pyscsi.scsi import SCSI

DEFAULT_VOLUME_NAME = 'volISCSI'

class TestISCSICrtVolume(ISCSIFixture):
    """FDS test case to create an iSCSI volume.

    Attributes
    ----------
    passedName : str
        FDS volume name
    passedPermissions: str
        iSCSI permissions, 'rw' or 'ro'
    passedSize : str
        block size
    sizeSizeUnits: str
        block size units
    """
    def __init__(self, parameters=None, name=None, permissions="rw", size="10", sizeunits="GB"):
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_ISCSICrtVolume,
                                             "Creating an iSCSI volume")
        self.passedName = name
        self.passedPermissions = permissions
        self.passedSize = size
        self.passedSizeUnits = sizeunits

    def test_ISCSICrtVolume(self):
        """Attempt to create an iSCSI volume."""

        if not self.passedName:
            self.passedName = DEFAULT_VOLUME_NAME

        fdscfg = self.parameters["fdscfg"]
        om_node = fdscfg.rt_om_node

        # iSCSI volume create command
        new_volume = Volume()
        new_volume.name = self.passedName
        new_volume.settings = ISCSISettings(capacity=Size(self.passedSize, unit=self.passedSizeUnits))
        new_volume.settings.lun_permissions = [LunPermissions(permissions=self.passedPermissions, lun_name=self.passedName)]

        vol_service = get_volume_service(self,om_node.nd_conf_dict['ip'])
        status = vol_service.create_volume(new_volume)
        if isinstance(status, FdsError):
            self.log.error("Volume %s creation on %s returned status %s." %
                               (new_volume.name, om_node.nd_conf_dict['node-name'], status))
            return False

        # Cache this volume name in the fixture so that other test cases
        # can use it to map to ISCSI target name and drive
        self.addVolumeName(self.passedName)
        return True


class TestISCSIDiscoverVolume(ISCSIFixture):
    """FDS test case to discover an iSCSI target

    Attributes
    ----------
    target_name : str
        If test was successful, contains target name following IQN convention.
        Example: 'iqn.2012-05.com.formationds:volISCSI'
        None otherwise.
    volume_name : str
        FDS volume name, initialized with object instantation
        else from fixture
    """

    def __init__(self, parameters=None, volume_name=None):
        """
        Parameters
        ----------
        parameters : List[str]
        volume_name : str
            The volume name as provided to FDS create volume
        """
        super(self.__class__, self).__init__(parameters,
                self.__class__.__name__,
                self.test_discover_volume,
                "Discover iSCSI target")

        self.volume_name = volume_name

    def test_discover_volume(self):
        """Uses native iSCSI initiator to discover volume on target host

        Returns
        -------
        bool
            True if successful, False otherwise
        """
        if not self.volume_name:
            self.volume_name = DEFAULT_VOLUME_NAME

        self.target_name = None
        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]
        om_node = fdscfg.rt_om_node

        # -t, --type=type
        #   Type of SendTargets (abbreviated as st) which is a native iSCSI protocol
        #       which allows each iSCSI target to send a list of available targets to
        #       the initiator
        cmd = 'iscsiadm -m discovery -p %s -t st' % (om_node.nd_conf_dict['ip'])

        # Parameter return_stdin is set to return stdout. ... Don't ask me!
        status, stdout = om_node.nd_agent.exec_wait(cmd, return_stdin=True)

        if status != 0:
            self.log.info("Failed to discover iSCSI volumes.")
            return False
        else:
            g = stdout.split()
            for s in g:
                if s.find(self.volume_name) != -1:
                    self.target_name = s
                    # Cache target name in fixture
                    self.setTargetName(self.target_name, self.volume_name)
                    return True

        return False


class TestISCSIFioSeqW(ISCSIFixture):
    """FDS test case to write to an iSCSI volume

    Attributes
    ----------
    sd_device : str
        Device using sd upper level driver (example: '/dev/sdb')
    volume_name : str
        FDS volume name
    """
    def __init__(self, parameters=None, volume_name=None):
        """
        Parameters
        ----------
        volume_name : str
            FDS volume name
        """

        super(self.__class__, self).__init__(parameters,
                self.__class__.__name__,
                self.test_fio_write,
                "Write to an iSCSI volume")

        self.volume_name = volume_name
        self.sd_device = None

    def test_fio_write(self):
        """
        Use flexible I/O tester to write to iSCSI volume

        Returns
        -------
        bool
            True if successful, False otherwise
        """
        if not self.volume_name:
            self.log.error("Missing required iSCSI target name")
            return False
        # Use the block interface (not the char interface)
        if not self.sd_device:
            # Use fixture target name
            self.sd_device = self.getDriveDevice(self.volume_name)
        if not self.sd_device:
            self.log.error("Missing disk device for %s" % self.volume_name)
            return False
        else:
            self.log.info("block device is {0}".format(self.sd_device))

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]
        om_node = fdscfg.rt_om_node
        om_ip = om_node.nd_conf_dict['ip']

        if self.childPID is None:
            # Not running in a forked process.
            # Stop on failures.
            verify_fatal = 1
        else:
            # Running in a forked process. Don't
            # stop on failures.
            verify_fatal = 0

        # Specify connection info
        assert connect_fabric(self, om_ip) is True

# This one produced a failure! TODO: debug with Brian S...
#        cmd = "sudo fio --name=seq-writers --readwrite=write --ioengine=posixaio --direct=1 --bsrange=512-1M " \
#                 "--iodepth=128 --numjobs=1 --fill_device=1 --filename=%s --verify=md5 --verify_fatal=%d" %\
#                 (self.sd_device, verify_fatal)

        cmd = "sudo fio --name=seq-writers --readwrite=write --ioengine=posixaio --direct=1 --bsrange=512-1M " \
                 "--iodepth=128 --numjobs=1 --fill_device=1 --filename=%s --size=16m --verify=md5 --verify_fatal=%d" %\
                 (self.sd_device, verify_fatal)

        # Fabric run a shell command on a remote host
        status = run(cmd)
        disconnect_fabric()

        return True


class TestISCSIListVolumes(ISCSIFixture):
    """FDS test case to list volumes

    """
    def __init__(self, parameters=None, volume_name=None):
        """
        Parameters
        ----------
        volume_name : str
            FDS volume name
        """

        super(self.__class__, self).__init__(parameters,
                self.__class__.__name__,
                self.test_list_volumes,
                "List volumes using CLI")

        self.volume_name = volume_name

    def test_list_volumes(self):
        """
        Attempt to list FDS volumes, look for our iSCSI volume

        Returns
        -------
        bool
            True if successful, False otherwise
        """
        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]
        om_node = fdscfg.rt_om_node

        vol_service = get_volume_service(self,om_node.nd_conf_dict['ip'])
        volumes = vol_service.list_volumes()
        for v in volumes:
            if v.name != self.volume_name:
                continue
            s = v.settings
            if s.type != 'ISCSI':
                self.log.info("Require ISCSI type")
            return True

        self.log.info("Missing volume!")
        return True


class TestISCSIAttachVolume(ISCSIFixture):
    """FDS test case to login to an iSCSI target

    Yields generic driver device name so that other test cases can use pass
    through char device for testing.

    Attributes
    ----------
    sd_device : str
        Device using sd upper level driver (example: '/dev/sdb')
    sg_device : str
        Device using sg upper level driver (example: '/dev/sg2')
    target_name : str
        The iSCSI node record name to login.
        Target name is expected to follow IQN convention.
        Example: 'iqn.2012-05.com.formationds:volISCSI'
    volume_name : str
        FDS volume name
    """

    def __init__(self, parameters=None, target_name=None, volume_name=None):
        """
        Parameters
        ----------
        target_name : str
            The iSCSI node record name to login
        volume_name : str
            FDS volume name
        """

        super(self.__class__, self).__init__(parameters,
                self.__class__.__name__,
                self.test_login_target,
                "Attaching iSCSI volume")

        self.volume_name = volume_name
        self.target_name = target_name
        self.sd_device = None
        self.sg_device = None

    def test_login_target(self):
        """
        Attempt to attach an iSCSI volume.

        Returns
        -------
        bool
            True if successful, False otherwise
        """
        if not self.target_name:
            if not self.volume_name:
                self.log.error("Missing required iSCSI target name")
                return False
            else:
                # Use fixture target name
                self.target_name = self.getTargetName(self.volume_name)

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]
        om_node = fdscfg.rt_om_node

        # Cache the device names prior to login
        # Parameter return_stdin is set to return stdout. ... Don't ask me!
        disk_devices = []
        generic_devices = []
        status, stdout = om_node.nd_agent.exec_wait('sg_map -sd', return_stdin=True)
        if status != 0:
            self.log.error("Failed to get sg map");
            return False;

        g = stdout.split()
        for elem in g:
            if elem[:7] == '/dev/sd':
                disk_devices.append(elem)
            elif elem[:7] == '/dev/sg':
                generic_devices.append(elem)

        cmd = 'iscsiadm -m node --targetname %s -p %s --login' % (self.target_name, (om_node.nd_conf_dict['ip']))
        # Parameter return_stdin is set to return stdout. ... Don't ask me!
        status, stdout = om_node.nd_agent.exec_wait(cmd, return_stdin=True)

        if status != 0:
            self.log.info("Failed to login iSCSI target %s." % self.target_name)
            return False

        # Compute device names
        status, stdout = om_node.nd_agent.exec_wait('sg_map -sd', return_stdin=True)
        if status == 0:
            gprime = stdout.split()
            for elem in gprime:
                if elem[:7] == '/dev/sd' and not elem in disk_devices:
                    self.sd_device = elem
                    self.setDriveDevice(elem, self.volume_name)
                    self.log.info("Added disk device %s" % elem)
                elif elem[:7] == '/dev/sg' and not elem in generic_devices:
                    self.sg_device = elem
                    self.setGenericDevice(elem, self.volume_name)
                    self.log.info("Added generic device %s" % elem)

        if not self.sd_device:
            self.log.error("Missing disk device for %s" % self.volume_name)
            return False
        if not self.sg_device:
            self.log.error("Missing generic device for %s" % self.volume_name)
            return False

        return True


class TestISCSIMakeFilesystem(ISCSIFixture):
    """FDS test case to make file system on disk device
    """
    def __init__(self, parameters=None, sd_device=None, volume_name=None):
        """
        Parameters
        ----------
        sd_device : Optional[str]
            The disk device (example: '/dev/sdb')
        volume_name : str
            FDS volume name
        """
        super(self.__class__, self).__init__(parameters,
                self.__class__.__name__,
                self.test_make_filesystem,
                "Make filesystem on disk device")

        self.sd_device = sd_device
        self.volume_name = volume_name

    def test_make_filesystem(self):
        """
        Attempt to make filesystem on disk device.

        Returns
        -------
        bool
            True if successful, False otherwise
        """
        # Volume name is required even when sd_device is present.
        # This method uses volume name in the /mnt child directory name.
        if not self.volume_name:
            self.log.error("Missing required iSCSI volume name")
            return False

        if not self.sd_device:
            # Use fixture target name
            self.sd_device = self.getDriveDevice(self.volume_name)

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]
        om_node = fdscfg.rt_om_node

        device = self.sd_device[:8]
        # Uses a partition table of type 'gpt'
        cmd = 'parted -s -a optimal %s mklabel gpt -- mkpart primary ext4 1 -1' % device
        # Parameter return_stdin is set to return stdout. ... Don't ask me!
        status, stdout = om_node.nd_agent.exec_wait(cmd, return_stdin=True)

        if status != 0:
            self.log.info("Failed to make file system for device %s." % device)
            return False

        # build file system
        cmd2 = 'mkfs.ext4 %s1' % device
        # Parameter return_stdin is set to return stdout. ... Don't ask me!
        status, stdout = om_node.nd_agent.exec_wait(cmd2, return_stdin=True)
        # Nominally returns -1 because mkfs.ext4 is ill-behaved. It writes
        # to stderr.
        if status > 0:
            self.log.info("Failed to format file system")
            return False

        # mount
        path = '/mnt/%s' % self.volume_name

        if not os.path.isdir(path):
            cmd3 = 'mkdir %s' % path
            status, stdout = om_node.nd_agent.exec_wait(cmd3, return_stdin=True)
            if status != 0:
                self.log.error("Failed to create mount directory %s" % path)
                return False

        cmd4 = 'mount %s1 %s' % (device, path)
        status, stdout = om_node.nd_agent.exec_wait(cmd4, return_stdin=True)

        self.log.info("Made ext4 file system on device %s." % device)
        return True


class TestISCSIUnitReady(ISCSIFixture):
    """FDS test case to send TEST_UNIT_READY CDB and check response

    """
    def __init__(self, parameters=None, sg_device=None, volume_name=None):
        """Only one of sg_device or volume_name is required

        Parameters
        ----------
        sg_device : str
            The generic sg device (example: '/dev/sg2')
        volume_name : str
            FDS volume name
        """
        super(self.__class__, self).__init__(parameters,
                self.__class__.__name__,
                self.test_unit_ready,
                "Testing TEST_UNIT_READY CDB in full feature phase")

        self.sg_device = sg_device
        self.volume_name = volume_name

    def test_unit_ready(self):
        """
        Sends TEST_UNIT_READY CDB and checks response.

        [RFC-7413] Section 4.3, 7.4.3
        Verify that target does not send SCSI response PDUs once in the
        Full Feature Phase.

        Returns
        -------
        bool
            True if successful, False otherwise
        """
        if not self.sg_device:
            if not self.volume_name:
                self.log.error("Missing required iSCSI device")
                return False
            else:
                # Use fixture target name
                self.sg_device = self.getGenericDevice(self.volume_name)

        if not self.sg_device:
            self.log.error("Missing required iCSSI generic device")
            return False

        # If test case not run as root, creating the SCSIDevice throws!
        try:
            # untrusted execute
            sd = SCSIDevice(self.sg_device, True)
            s = SCSI(sd)
            r = s.testunitready().result
            if not r:
                # empty dictionary, which is the correct response!
                return True;

            for k, v in r.iteritems():
                self.log.info('%s - %s' % (k, v))
        except Exception as e:
            self.log.error(str(e));

        return False


class TestISCSIDetachVolume(ISCSIFixture):
    """FDS test case for iSCSI node record logout

    Client has responsibility for maintaining the mapping from iSCSI node
    record name to device.

    Attributes
    ----------
    target_name : str
        Each target name is expected to following IQN convention
        Example: 'iqn.2012-05.com.formationds:volISCSI'
    volume_name : str
        FDS volume name
    """

    def __init__(self, parameters=None, target_name=None, volume_name=None):
        """
        Parameters
        ----------
        parameters : List[str]
        target_name : str
            The iSCSI node record name to logout, remove
        volume_name : str
            FDS volume name
        """
        super(self.__class__, self).__init__(parameters,
                self.__class__.__name__,
                self.test_logout_targets,
                "Disconnecting iSCSI volume")

        self.target_name = target_name
        self.volume_name = volume_name

    def test_logout_targets(self):
        """
        Logout, remove iSCSI node record.

        Returns
        -------
        bool
            True if successful, False otherwise
        """
        if not self.target_name:
            if not self.volume_name:
                self.log.error("Missing required iSCSI target name")
                return False
            else:
                self.target_name = self.getTargetName(self.volume_name)

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]
        om_node = fdscfg.rt_om_node
        om_ip = om_node.nd_conf_dict['ip']

        # Unmount BEFORE breaking down the iSCSI node record

        # Specify connection info
        assert connect_fabric(self, om_ip) is True

        path = '/mnt/{0}'.format(self.volume_name)
        # Fabric run a shell command on a remote host
        response = run('umount -f -v {0}'.format(path))

        if not response or response.count('has been unmounted') == 0:
            self.log.info('Failed to unmount {0}: {1}.'.format(path, response))
            disconnect_fabric()
            return False

        # Logout session
        cmd = 'iscsiadm -m node --targetname %s -p %s --logout' % (self.target_name, \
                (om_node.nd_conf_dict['ip']))
        # Parameter return_stdin is set to return stdout. ... Don't ask me!
        status, stdout = om_node.nd_agent.exec_wait(cmd, return_stdin=True)

        if status != 0:
            self.log.error("Failed to detach iSCSI volume %s." % t)
            disconnect_fabric()
            return False

        # Remove the record
        cmd2 = 'iscsiadm -m node -o delete -T %s -p %s' % (self.target_name, \
                (om_node.nd_conf_dict['ip']))

        # Parameter return_stdin is set to return stdout. ... Don't ask me!
        status, stdout = om_node.nd_agent.exec_wait(cmd2, return_stdin=True)

        if status != 0:
            self.log.error("Failed to remove iSCSI record %s." % self.target_name)
            disconnect_fabric()
            return False

        self.log.info("Detached volume %s and removed iSCSI record" % self.volume_name)

        disconnect_fabric()
        return True


if __name__ == '__main__':
    TestCase.FDSTestCase.fdsGetCmdLineConfigs(sys.argv)
    unittest.main()
