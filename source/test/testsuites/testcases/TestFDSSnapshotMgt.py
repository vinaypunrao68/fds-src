#!/usr/bin/python
#
# Copyright 2014 by Formation Data Systems, Inc.
#

# FDS test-case pattern requirements.
import pdb
import unittest
import traceback

import xmlrunner
import TestCase
from fdslib import SvcHandle

# Module-specific requirements
import sys
from fdslib.TestUtils import get_volume_service
from fdscli.model.volume.snapshot import Snapshot
from fdscli.model.fds_error import FdsError


# This class contains the attributes and methods to test
# create snapshot
class TestCreateSnapshot(TestCase.FDSTestCase):
    def __init__(self, parameters=None, volume_name=None, snapshot_name=None, retention =None):
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_CreateSnapshot,
                                             "Create Snapshot")

        self.passedVolume_name = volume_name
        self.passedSnapshot_name = snapshot_name
        self.passedRetention = retention

    def test_CreateSnapshot(self):

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]
        om_node = fdscfg.rt_om_node
        vol_service = get_volume_service(self, om_node.nd_conf_dict['ip'])
        volume_list = vol_service.list_volumes()

        for each_volume in volume_list:
            if self.passedVolume_name == each_volume.name:
                vol_snapshot = Snapshot()
                vol_snapshot.volume_id = each_volume.id
                vol_snapshot.name = self.passedSnapshot_name
                vol_snapshot.retention = self.passedRetention
                status = vol_service.create_snapshot(vol_snapshot)

                snapshot_list = vol_service.list_snapshots(vol_snapshot.volume_id)
                for snapshot in snapshot_list:
                    if snapshot.volume_id == vol_snapshot.volume_id:
                        self.log.info("Snapshot created for volume name={}: {}".format(self.passedVolume_name, snapshot.name))

                if isinstance(status, FdsError) or type(status).__name__ == 'FdsError':
                    self.log.error("FAILED:  Failing to create volume snapshot for volume {%s}" %self.passedVolume_name)
                    return False
                elif self.passedVolume_name is not None:
                    # Passed a specific node so get out.
                    break

        return True

# This class contains the attributes and methods to test
# create snapshot
class TestListSnapshot(TestCase.FDSTestCase):
    def __init__(self, parameters=None, volume=None):
        """
        When run by a qaautotest module test runner,
        "parameters" will have been populated with
        .ini configuration.
        """
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_ListSnapshots,
                                             "List Snapshots")

        self.passedVolume = volume

    def test_ListSnapshots(self):
        """
        Test Case:
        Attempt to List snapshot
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]
        om_node = fdscfg.rt_om_node
        volumes = fdscfg.rt_get_obj('cfg_volumes')
        vol_service = get_volume_service(self, om_node.nd_conf_dict['ip'])
        volume_list = vol_service.list_volumes()

        for volume in volumes:
            # If we were passed a node, use it and get out. Otherwise,
            # we boot all DMs we have.
            if self.passedVolume is not None:
                volume = self.passedVolume

            if 'id' not in volume.nd_conf_dict:
                raise Exception('Volume section %s must have "id" keyword.' % volume.nd_conf_dict['vol-name'])

            self.log.info("Creating snapshot for volume {} on OM node {}".format(volume.nd_conf_dict['vol-name'],
                        om_node.nd_conf_dict['ip']))

        for eachvolume in volume_list:
            if volume.nd_conf_dict['vol-name'] == eachvolume.name:

                snapshot_list = vol_service.list_snapshots(eachvolume.id)

                for snapshot in snapshot_list:
                    if snapshot.volume_id == eachvolume.id:
                        self.log.info("Snapshot found a match for volume={}: {}".format(volume.nd_conf_dict['vol-name'], snapshot.name))

                if isinstance(snapshot_list, FdsError):
                    self.log.error("FAILED:  Failing to create volume snapshot for volume {}" %(volume.nd_conf_dict['vol-name'], snapshot_list))
                    return False
                elif self.passedVolume is not None:
                    # Passed a specific node so get out.
                    break

        return True


if __name__ == '__main__':
    TestCase.FDSTestCase.fdsGetCmdLineConfigs(sys.argv)
    unittest.main(testRunner=xmlrunner.XMLTestRunner(output='test-reports'))
