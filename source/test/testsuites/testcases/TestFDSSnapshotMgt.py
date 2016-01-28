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
import random
import datetime
# Module-specific requirements
import sys
from fdslib.TestUtils import get_volume_service
from fdscli.model.volume.snapshot import Snapshot
from fdscli.model.fds_error import FdsError


# This class contains the attributes and methods to test
# create snapshot
class TestCreateSnapshot(TestCase.FDSTestCase):
    def __init__(self, parameters=None, volume_name=None, snapshot_name=None, retention=None):
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
                        self.log.info(
                            "Snapshot created for volume name={}: {}".format(self.passedVolume_name, snapshot.name))

                if isinstance(status, FdsError) or type(status).__name__ == 'FdsError':
                    self.log.error(
                        "FAILED:  Failing to create volume snapshot for volume {%s}" % self.passedVolume_name)
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
                        self.log.info("Snapshot found a match for volume={}: {}".format(volume.nd_conf_dict['vol-name'],
                                                                                        snapshot.name))

                if isinstance(snapshot_list, FdsError):
                    self.log.error("FAILED:  Failing to create volume snapshot for volume {}" % (
                    volume.nd_conf_dict['vol-name'], snapshot_list))
                    return False
                elif self.passedVolume is not None:
                    # Passed a specific node so get out.
                    break

        return True


# If neither of snapshot1 and snapshot2 creation time is given -> timeline is current time
# If only either is passed -> timeline is (given) snapshot creation time
# If range of two snapshots is passed -> timeline is between first and second snapshot
class TestCreateVolClone(TestCase.FDSTestCase):
    def __init__(self, parameters=None, volume_name=None, clone_name=None, snapshot_start=None, snapshot_end=None):
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_CreateVolClone,
                                             "Create Snapshot")

        self.passedVolume_name = volume_name
        self.passedClone_name = clone_name
        self.passedSnapshort_start = snapshot_start
        self.passedSnapshort_end = snapshot_end

    def test_CreateVolClone(self):

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]
        om_node = fdscfg.rt_om_node
        vol_service = get_volume_service(self, om_node.nd_conf_dict['ip'])
        each_volume = vol_service.find_volume_by_name(self.passedVolume_name)
        snapshot_list = vol_service.list_snapshots(each_volume.id)
        print  datetime.datetime.now().time()
        if self.passedSnapshort_start is None and self.passedSnapshort_end is None:
            timeline = datetime.datetime.now().time()
            print 'Creating clone in case1 at' + timeline

        elif self.passedSnapshort_start is None or self.passedSnapshort_end is None:
            for snapshot in snapshot_list:
                if snapshot.name == self.passedSnapshort_start:
                    timeline = snapshot.created
                elif snapshot.name == self.passedSnapshort_end:
                    timeline = snapshot.created
            assert timeline is not None
            print 'Creating clone in case2 at' + timeline

        elif self.passedSnapshort_start is not None and self.passedSnapshort_end is not None:
            for snapshot in snapshot_list:
                if snapshot.name == self.passedSnapshort_start:
                    time_start = snapshot.created
                if snapshot.name == self.passedSnapshort_end:
                    time_end = snapshot.created
            assert time_end > time_start
            assert (time_start and time_end) is not None

            timeline = random.randrange(time_start, time_end, 2)
            print 'Creating clone in case3 at' + timeline
        status = vol_service.clone_from_timeline(each_volume, timeline)

        return True


if __name__ == '__main__':
    TestCase.FDSTestCase.fdsGetCmdLineConfigs(sys.argv)
    unittest.main(testRunner=xmlrunner.XMLTestRunner(output='test-reports'))
