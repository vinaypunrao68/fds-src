#!/usr/bin/python
#
# Copyright 2016 by Formation Data Systems, Inc.
#

# FDS test-case pattern requirements.
import unittest
import TestCase

# Module-specific requirements
import sys
from fdscli.model.volume.settings.iscsi_settings import ISCSISettings
from fdscli.model.volume.settings.lun_permission import LunPermissions
from fdscli.model.common.size import Size
from fdscli.model.volume.volume import Volume
from fdslib.TestUtils import get_volume_service
from fdscli.model.fds_error import FdsError

# This class contains the attributes and methods to test
# the FDS interface to create an iSCSI volume.
#
class TestISCSICrtVolume(TestCase.FDSTestCase):
    def __init__(self, parameters=None, name="volISCSI", permissions="rw", size="10", sizeunits="GB"):
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_ISCSICrtVolume,
                                             "Creating an iSCSI volume")
        self.passedName = name
        self.passedPermissions = permissions
        self.passedSize = size
        self.passedSizeUnits = sizeunits

    def test_ISCSICrtVolume(self):
        """
        Test Case:
        Attempt to create an iSCSI volume.
        """

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

        return True


if __name__ == '__main__':
    TestCase.FDSTestCase.fdsGetCmdLineConfigs(sys.argv)
    unittest.main()
