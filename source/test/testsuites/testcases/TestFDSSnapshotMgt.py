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
import time
# Module-specific requirements
import sys
from fdslib.TestUtils import get_volume_service
from fdscli.model.volume.snapshot import Snapshot
from fdscli.model.fds_error import FdsError
from fdslib.TestUtils import create_fdsConf_file
import datetime
from fabric.contrib.files import *
from fdslib.TestUtils import connect_fabric
from fdslib.TestUtils import disconnect_fabric

from fabric.api import local
from fdslib.TestUtils import verify_disk_free

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
                # from fs-4999 when the disk space reaches 75%, snapshots should NOT happen.
                # Threshhold might change in future
                #assert verify_disk_free(self,75) is True
                status = vol_service.create_snapshot(vol_snapshot)

                snapshot_list = vol_service.list_snapshots(vol_snapshot.volume_id)
                for snapshot in snapshot_list:
                    if snapshot.volume_id == vol_snapshot.volume_id:
                        self.log.info(
                            "Snapshot {0} created for volume {1} at time {2}".format(self.passedVolume_name, snapshot.name, snapshot.created))

                if isinstance(status, FdsError) or type(status).__name__ == 'FdsError':
                    self.log.error(
                        "FAILED:  Failing to create volume snapshot for volume {%s}" % self.passedVolume_name)
                    return False
                elif self.passedVolume_name is not None:
                    # Passed a specific node so get out.
                    break

        # added sleep so that we have have significant time between consecutive snapshots
        time.sleep(5)
        return True


# This class contains the attributes and methods to test
# list snapshots of a passed volume
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
            # If we were passed a node, use it and get out.
            if self.passedVolume is not None:
                volume = self.passedVolume

            if 'id' not in volume.nd_conf_dict:
                raise Exception('Volume section %s must have "id" keyword.' % volume.nd_conf_dict['vol-name'])

        for each_volume in volume_list:
            if volume.nd_conf_dict['vol-name'] == each_volume.name:

                snapshot_list = vol_service.list_snapshots(each_volume.id)

                for snapshot in snapshot_list:
                    if snapshot.volume_id == each_volume.id:
                        self.log.info("Snapshot found for volume={}: {}".format(volume.nd_conf_dict['vol-name'],
                                                                                        snapshot.name))

                if isinstance(snapshot_list, FdsError):
                    self.log.error("FAILED: To list snapshots for volume %s" % (
                        volume.nd_conf_dict['vol-name']))
                    return False
                elif self.passedVolume is not None:
                    # Passed a specific node so get out.
                    break

        return True


# This class contains the attributes and methods to test
# volume clone from timeline.
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
        passed_volume = vol_service.find_volume_by_name(self.passedVolume_name)
        snapshot_list = vol_service.list_snapshots(passed_volume.id)
        timeline = None

        # Case1: neither of snapshot1 and snapshot2 are passed -> timeline = now
        if self.passedSnapshort_start is None and self.passedSnapshort_end is None:
            # pass 0 as timeline to clone volume with current timestamp
            timeline = 0

        # Case2: only one snapshot is passed -> snapshot_start/end < timeline < now
        elif self.passedSnapshort_start is None or self.passedSnapshort_end is None:
            time_start = 0
            for snapshot in snapshot_list:
                if snapshot.name == self.passedSnapshort_start:
                    time_start = snapshot.created
                elif snapshot.name == self.passedSnapshort_end:
                    time_start = snapshot.created
            now = int(time.time())
            # FIXME : fs-5171
            timeline = random.randrange(time_start + 2, now, 1)


        # Case3:Two snapshots are passed -> snapshot_start < timeline < snapshot_end
        elif self.passedSnapshort_start is not None and self.passedSnapshort_end is not None:
            time_start = 0
            time_end = 0
            for snapshot in snapshot_list:
                if str(snapshot.name) == self.passedSnapshort_start:
                    time_start = snapshot.created
                if str(snapshot.name) == self.passedSnapshort_end:
                    time_end = snapshot.created
            assert time_end > time_start
            # FIXME : fs-5171
            timeline = random.randrange(time_start + 2, time_end, 1)

        assert timeline is not None
        create_fdsConf_file(om_node.nd_conf_dict['ip'])
        cmd = 'fds volume clone -name {0} -volume_id {1} -time {2}'.format(self.passedClone_name, passed_volume.id, timeline)
        status = local(cmd, capture=True)
        time.sleep(3)  # let clone volume creation propogate
        cloned_volume = vol_service.find_volume_by_name(self.passedClone_name)

        if type(cloned_volume).__name__ == 'FdsError':
            self.log.error("Creating %s clone from timeline failed" %self.passedVolume_name)
        elif type(cloned_volume).__name__ == 'Volume':
            self.log.info("Created clone {0} of volume {1} with time line {2}".format(self.passedClone_name, self.passedVolume_name, timeline))
            return True

        self.log.error("Creating %s clone from timeline failed" %self.passedVolume_name)

        return False


# This class contains the attributes and methods to test timeline.
# We change system dates to verify if daily,weekly, monthly and yearly snapshots do happen.
class TestTimeline(TestCase.FDSTestCase):
    def __init__(self, parameters=None):
        """
        When run by a qaautotest module test runner,
        "parameters" will have been populated with
        .ini configuration.
        """
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_timeline,
                                             "Test Time line with daily monthly and yearly snapshots")

    def test_timeline(self):
        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]
        nodes = fdscfg.rt_obj.cfg_nodes
        om_node = fdscfg.rt_om_node

        # For now , check only for daily,weekly,monthly and yearly snapshot creation
        number_of_days = [1, 7, 30, 365]
        patterns = ['daily', 'weekly', 'monthly', 'yearly']

        vol_service = get_volume_service(self, om_node.nd_conf_dict['ip'])
        volume_list = vol_service.list_volumes()
        for idx, val in enumerate(number_of_days):
            change_date(self, val)
            if self.parameters['ansible_install_done']:
                change_date(self, val, nodes)

            # after changing dates sleep for 5 secs before checking if snapshots are available
            time.sleep(5)

            for each_volume in volume_list:
                pattern = 'snap.' + str(each_volume.name) + '.' + str(each_volume.id) + '_system_timeline_' + patterns[idx]
                if snapshot_created(self, each_volume, pattern)is not True:
                    self.log.error("FAILED: couldn't find {0} snapshot for vol_name = {1} , vol_id = {2}".format(pattern[idx], each_volume.name,each_volume.id))

                    # Even if it's failure, sync up time on each node using ntp
                    sync_time_with_ntp(self)
                    if self.parameters['ansible_install_done']:
                        sync_time_with_ntp(self, nodes)
                    return False

        # Before returning success, sync time on all nodes using network time protocol
        if self.parameters['ansible_install_done']:
            sync_time_with_ntp(self, nodes)
        sync_time_with_ntp(self)
        return True

# This method returns current date from node
# @param node_ip: ip address of the node. If node_ip is passed then return date on that node
# else date on local machine
def get_date_from_node(self, node_ip=None):
    if node_ip is None:
        with hide('output','running'):
            string_date = local("date +\"%Y-%m-%d %H:%M:%S\"", capture=True)
            current_time = datetime.datetime.strptime(string_date, "%Y-%m-%d %H:%M:%S")
            return current_time
    else:
        connect_fabric(self, node_ip)
        string_date = run("date +\"%Y-%m-%d %H:%M:%S\"", quiet=True)
        current_time = datetime.datetime.strptime(string_date, "%Y-%m-%d %H:%M:%S")
        disconnect_fabric()
        return current_time


# This method changes date to future date on all nodes in the domain.
# @param nodes: change dates on each node of the domain.
# @param number_of_days: change date to current date + number_of_days
def change_date(self, number_of_days, nodes=None):
    if nodes is None:  # all nodes running on same machine
        new_date = get_date_from_node(self) + datetime.timedelta(days=number_of_days)
        cmd = "sudo date --set=\"{0}\"  && date --rfc-3339=ns".format(new_date)
        with hide('output','running'):
            op = local(cmd, capture=False)
        self.log.info("Changed date on local node :{0}".format(get_date_from_node(self)))

    else:
        for node in nodes:
            connect_fabric(self, node.nd_host)
            new_date = str(get_date_from_node(self, node.nd_host) + datetime.timedelta(days=number_of_days))

            # set the date on remote node using below command
            cmd = "date --set=\"{0}\"  && date --rfc-3339=ns".format(new_date)
            op = sudo(cmd, quiet=True)
            self.log.info("Changed date on node {0} :{1}".format(node.nd_host, get_date_from_node(self, node.nd_host)))
            disconnect_fabric()
    return True


def snapshot_created(self, volume, pattern):
    fdscfg = self.parameters["fdscfg"]
    om_node = fdscfg.rt_om_node
    vol_service = get_volume_service(self, om_node.nd_conf_dict['ip'])
    snapshot_list = vol_service.list_snapshots(volume.id)
    self.log.info(
        'Found number_of_snapshots = {0}, vol_name = {1}, vol_id = {2}'.format(len(snapshot_list), volume.name, volume.id))
    found_snapshot = False
    for snapshot in snapshot_list:
        snapshot_name = snapshot.name
        # TODO pooja: after fs-5246 is fixed put check on number of snapshots created.
        if snapshot_name.startswith(pattern):
            found_snapshot = True
            self.log.info("OK: {0} is available for {1} , vol_id = {2}".format(snapshot_name, volume.name, volume.id))
            break

    return found_snapshot


# This method syncs time using ntp. Irrespective of timeline testing usccess /failure sync time with ntp
# making sure domain is in good state.
# @param nodes: is none if all nodes in domain are running on same machine
# @param nodes: is NOT none if every node in domain is running on different machine
def sync_time_with_ntp(self, nodes=None):
    cmd = "ntpdate ntp.ubuntu.com"
    if nodes is None:
        with hide('output','running'):
            local("sudo "+cmd, capture=True)
            self.log.info("Sync time using ntp on local machine. Now date is {0}".format(get_date_from_node(self)))
            return True
    else:
        for node in nodes:
            connect_fabric(self, node.nd_host)
            op = sudo(cmd, quiet=True)
            self.log.info("Sync time using ntp on AWS node {0}. Now date is {1}".format(node.nd_host,get_date_from_node(self, node.nd_host)))
            disconnect_fabric()
        return True


if __name__ == '__main__':
    TestCase.FDSTestCase.fdsGetCmdLineConfigs(sys.argv)
    unittest.main(testRunner=xmlrunner.XMLTestRunner(output='test-reports'))
