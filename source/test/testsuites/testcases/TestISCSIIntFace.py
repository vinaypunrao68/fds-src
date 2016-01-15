#!/usr/bin/python
#
# Copyright 2016 by Formation Data Systems, Inc.
#

# FDS test-case pattern requirements.
import unittest
import TestCase
# This file wants to subclass the ISCSIFixture class, not the
# ISCSIFixture module.
from testcases.iscsifixture import ISCSIFixture

# Module-specific requirements
import sys
from fdscli.model.volume.settings.iscsi_settings import ISCSISettings
from fdscli.model.volume.settings.lun_permission import LunPermissions
from fdscli.model.common.size import Size
from fdscli.model.volume.volume import Volume
from fdslib.TestUtils import get_volume_service
from fdscli.model.fds_error import FdsError

# A third-party initiator that enables sending CDBs and fetching response
from pyscsi.pyscsi.scsi_device import SCSIDevice

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
            # If not provided, fetch volume name from fixture
            self.passedName = self.getVolumeName()

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
        self.setVolumeName(self.passedName)
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
            # If not provided, fetch volume name from fixture
            self.volume_name = self.getVolumeName()

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


class TestISCSIAttachVolume(ISCSIFixture):
    """FDS test case to login to an iSCSI target

    Yields generic driver device name so that other test cases can use pass
    through char device for testing.

    TODO: consider yielding sd_device for block testing.

    Attributes
    ----------
    sg_device : str
        SCSI device name for the given iSCSI node record
        Example: '/dev/sg2'
    target_name : str
        The iSCSI node record name to login.
        Target name is expected to follow IQN convention.
        Example: 'iqn.2012-05.com.formationds:volISCSI'
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
        self.sg_device = None

    def test_login_target(self):
        """
        Attempt to attach an iSCSI volume.

        Returns
        -------
        bool
            True if successful, False otherwise
        """
        if not self.volume_name:
            # Use fixture target name
            self.volume_name = self.getVolumeName()

        if not self.target_name:
            # Use fixture target name
            self.target_name = self.getTargetName(self.volume_name)

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]
        om_node = fdscfg.rt_om_node

        cmd = 'iscsiadm -m node --targetname %s -p %s --login' % (self.target_name, (om_node.nd_conf_dict['ip']))
        return False
        # Parameter return_stdin is set to return stdout. ... Don't ask me!
#        status, stdout = om_node.nd_agent.exec_wait(cmd, return_stdin=True)

#        if status != 0:
#            self.log.info("Failed to login iSCSI target %s." % self.target_name)
#            return False

#        return True


class TestISCSIUnitReady(ISCSIFixture):
    """FDS test case to send TEST_UNIT_READY CDB and check response

    """
    def __init__(self, parameters=None, sg_device=None, volume_name=None):
        """
        Parameters
        ----------
        sg_device : str
            The generic sg device (example: '/dev/sg2')
        volume_name : str
            FDS volume name
        """

        super(self.__class__, self).__init__(parameters,
                self.__class__.__name__,
                self.test_login_target,
                "Testing TEST_UNIT_READY CDB")

        self.sg_device = sg_device
        self.volume_name = volume_name

    def test_unit_ready(self):
        """
        Sends TEST_UNIT_READY CDB and checks response.

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
        for volume in volumes:
            status = volume.status
            settings = volume.settings
            t = settings.type

        try:
            # untrusted execute
            sd = SCSIDevice(self.sg_device)
        except Exception as e:
#            e = sys.exc_info()[0]
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
        Example: ''
    """

    def __init__(self, parameters=None, target_name=None):
        """
        Parameters
        ----------
        parameters : List[str]
        target_name : str
            The iSCSI node record name to logout
        """
        super(self.__class__, self).__init__(parameters,
                self.__class__.__name__,
                self.test_logout_targets,
                "Disconnecting iSCSI volume")

        self.target_name = target_name

    def test_logout_targets(self):
        """
        Logout one or more iSCSI node records by name.

        Returns
        -------
        bool
            True if successful, False otherwise
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]
        om_node = fdscfg.rt_om_node

        cmd = 'iscsiadm -m node --targetname %s -p %s --logout' % self.target_name \
                % (om_node.nd_conf_dict['ip'])
        # Parameter return_stdin is set to return stdout. ... Don't ask me!
        status, stdout = om_node.nd_agent.exec_wait(cmd, return_stdin=True)

        if status != 0:
            self.log.error("Failed to detach iSCSI volume %s." % t)
            return False

        return True


if __name__ == '__main__':
    TestCase.FDSTestCase.fdsGetCmdLineConfigs(sys.argv)
    unittest.main()
