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
import time
import logging
import shlex
import random
import re
import os

import fdslib.platformservice as plat_svc
import fdslib.FDSServiceUtils as FDSServiceUtils


from fdslib.TestUtils import findNodeFromInv
from fdslib.TestUtils import check_localhost
from fdslib.TestUtils import get_volume_service
from fdslib.TestUtils import convertor

from fdscli.services.fds_auth import FdsAuth
from fdscli.services.node_service import NodeService
from fdscli.services.snapshot_service import SnapshotService
from fdscli.services.volume_service import VolumeService
from fdscli.services.users_service import UsersService
from fdscli.services.local_domain_service import LocalDomainService
from fdscli.services.snapshot_policy_service import SnapshotPolicyService

from fdscli.model.platform.node import Node
from fdscli.model.volume.volume import Volume
from fdscli.model.volume.snapshot import Snapshot
from fdscli.model.platform.domain import Domain
from fdscli.model.platform.service import Service
from fdscli.model.fds_error import FdsError



# This class contains the attributes and methods to test
# create snapshot
class TestCreateSnapshot(TestCase.FDSTestCase):
    def __init__(self, parameters=None, volume=None):
        """
        When run by a qaautotest module test runner,
        "parameters" will have been populated with
        .ini configuration.
        """
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_CreateSnapshot,
                                             "Create Snapshot")

        self.passedVolume = volume
        fdsauth = FdsAuth()
        fdsauth.login()

    def test_CreateSnapshot(self):
        """
        Test Case:
        Attempt to create snapshot
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
                newVolume = convertor(volume, fdscfg)
                newSnapshot = Snapshot(eachvolume.id)
                newSnapshot.volume_id = eachvolume.id
                newSnapshot.name = "{}_snapshot".format(eachvolume.name)
                status = vol_service.create_snapshot(newSnapshot)

                snapshot_list = vol_service.list_snapshots(newSnapshot.volume_id)

                for snapshot in snapshot_list:
                    if snapshot.volume_id == newSnapshot.volume_id:
                        self.log.info("Snapshot created for volume name={}: {}".format(volume.nd_conf_dict['vol-name'], snapshot.name))

                if isinstance(status, FdsError):
                    self.log.error("FAILED:  Failing to create volume snapshot for volume {}" %(volume.nd_conf_dict['vol-name'], status))
                    return False
                elif self.passedVolume is not None:
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
