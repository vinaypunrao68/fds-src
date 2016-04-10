#!/usr/bin/python
#
# Copyright 2014 by Formation Data Systems, Inc.
#

# FDS test-case pattern requirements.
from collections import defaultdict
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
from fdslib.TestUtils import execute_command_with_fabric
import datetime
import unittest

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
                # assert verify_disk_free(self,75) is True
                status = vol_service.create_snapshot(vol_snapshot)

                snapshot_list = vol_service.list_snapshots(vol_snapshot.volume_id)
                for snapshot in snapshot_list:
                    if snapshot.volume_id == vol_snapshot.volume_id:
                        self.log.info(
                            "Snapshot {0} created for volume {1} at time {2}".format(self.passedVolume_name,
                                                                                     snapshot.name, snapshot.created))

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
        cmd = 'fds volume clone -name {0} -volume_id {1} -time {2}'.format(self.passedClone_name, passed_volume.id,
                                                                           timeline)
        op = execute_command_with_fabric(cmd)
        time.sleep(3)  # let clone volume creation propogate
        cloned_volume = vol_service.find_volume_by_name(self.passedClone_name)

        if type(cloned_volume).__name__ == 'FdsError':
            self.log.error("Creating %s clone from timeline failed" % self.passedVolume_name)
        elif type(cloned_volume).__name__ == 'Volume':
            self.log.info("Created clone {0} of volume {1} with time line {2}".format(self.passedClone_name,
                                                                                      self.passedVolume_name, timeline))
            return True

        self.log.error("Creating %s clone from timeline failed" % self.passedVolume_name)

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

        # For now ,check only for daily,weekly,monthly and yearly snapshot creation
        number_of_days = [1, 7, 30, 365]

        vol_service = get_volume_service(self, om_node.nd_conf_dict['ip'])
        volume_list = vol_service.list_volumes()
        for day in number_of_days:
            # Store snapshot ids before changing date
            pre_date_change_ss_ids = self.store_latest_ss_id(om_node)
            change_date(self, day)
            if self.parameters['ansible_install_done']:
                change_date(self, day, nodes)
            # Sleep for 3 secs before checking if snapshots are available
            time.sleep(3)

            for each_volume in volume_list:
                pre_date_change_ss_id = pre_date_change_ss_ids[each_volume.id]
                if self.snapshot_created(each_volume, pre_date_change_ss_id) is not True:
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

    def store_latest_ss_id(self, om_node):
        vol_service = get_volume_service(self, om_node.nd_conf_dict['ip'])
        volume_list = vol_service.list_volumes()

        # "volume_ss_ids_dict" is dictionary which stores all snapshot ids for each volume,
        # key is volume id, value is list of snapshot ids for respective volume
        volume_ss_ids_dict = defaultdict(list)
        for each_vol in volume_list:
            ss_list = vol_service.list_snapshots(each_vol.id)
            for ss in ss_list:
                volume_ss_ids_dict[each_vol.id].append(int(ss.id.strip()))
        return volume_ss_ids_dict

    def snapshot_created(self, volume, pre_date_change_snapshots):
        fdscfg = self.parameters["fdscfg"]
        om_node = fdscfg.rt_om_node
        vol_service = get_volume_service(self, om_node.nd_conf_dict['ip'])
        snapshot_list = vol_service.list_snapshots(volume.id)
        post_date_change_snapshots = [int((snapshot.id).strip()) for snapshot in snapshot_list]
        post_date_change_snapshots_name = [snapshot.name for snapshot in snapshot_list]

        if len(pre_date_change_snapshots) == 0:
            assert len(post_date_change_snapshots) == 1
            self.log.info("{} is first snapshot for volume {}".format(post_date_change_snapshots, volume.id))
            return True

        # Check if new snapshot is created in retry loop.
        retryCount = 0
        maxRetries = 20
        while retryCount < maxRetries:
            retryCount += 1
            snapshot_list = vol_service.list_snapshots(volume.id)
            post_date_change_snapshots = [int((snapshot.id).strip()) for snapshot in snapshot_list]
            post_date_change_snapshots_name = [snapshot.name for snapshot in snapshot_list]

            if max(post_date_change_snapshots) > max(pre_date_change_snapshots):
                for snapshot in snapshot_list:
                    if int(snapshot.id.strip()) == max(post_date_change_snapshots):
                        ss = snapshot
                self.log.info('Success: New snapshot created = {0}, vol_name = {1}, vol_id = {2}'.
                          format(ss.name, volume.name, volume.id))
                return True
            elif retryCount < maxRetries:
                time.sleep(2)
                print "Waiting till new snapshot is active"
                continue

        self.log.error("Failed: No new snapshot found for volume {} with id = {}".format(volume.name, volume.id))
        self.log.error("Snapshot list for volume {} {}".format(volume.name,post_date_change_snapshots_name))
        return False


# This method returns current date from node
# @param node_ip: ip address of the node. If node_ip is passed then return date on that node
# else date on local machine
def get_date_from_node(self, node_ip=None):
    cmd = "date +\"%Y-%m-%d %H:%M:%S\""
    if node_ip is None:
        string_date = execute_command_with_fabric(cmd)
        current_time = datetime.datetime.strptime(string_date, "%Y-%m-%d %H:%M:%S")
        return current_time
    else:
        string_date = execute_command_with_fabric(cmd,remote_env=True, node_ip=node_ip)
        current_time = datetime.datetime.strptime(string_date, "%Y-%m-%d %H:%M:%S")
        return current_time


# This method changes date to future date on all nodes in the domain.
# @param nodes: change dates on each node of the domain.
# @param number_of_days: change date to current date + number_of_days
def change_date(self, number_of_days, nodes=None):
    if nodes is None:  # all nodes running on same machine
        new_date = get_date_from_node(self) + datetime.timedelta(days=number_of_days)
        cmd = "date --set=\"{0}\"  && date --rfc-3339=ns".format(new_date)
        op = execute_command_with_fabric(cmd, use_sudo=True)
        self.log.info("Changed date on local node :{0}".format(get_date_from_node(self)))
    else:
        for node in nodes:
            new_date = str(get_date_from_node(self, node.nd_host) + datetime.timedelta(days=number_of_days))
            # set the date on remote node using below command
            cmd = "date --set=\"{0}\"  && date --rfc-3339=ns".format(new_date)
            op = execute_command_with_fabric(cmd, use_sudo=True,remote_env=True, node_ip=node.nd_host)
            self.log.info("Changed date on node {0} :{1}".format(node.nd_host, get_date_from_node(self, node.nd_host)))
    return True


# This method syncs time using ntp. Irrespective of timeline testing usccess /failure sync time with ntp
# making sure domain is in good state.
# @param nodes: is none if all nodes in domain are running on same machine
# @param nodes: is NOT none if every node in domain is running on different machine
def sync_time_with_ntp(self, nodes=None):
    cmd = "ntpdate ntp.ubuntu.com"
    if nodes is None:
        execute_command_with_fabric(cmd,use_sudo=True)
        self.log.info("Sync time using ntp on local machine. Now date is {0}".format(get_date_from_node(self)))
    else:
        for node in nodes:
            execute_command_with_fabric(cmd, use_sudo=True, remote_env=True, node_ip=node.nd_host)
            self.log.info("Sync time using ntp on AWS node {0}. Now date is {1}".
                          format(node.nd_host,get_date_from_node(self, node.nd_host)))
    return True


if __name__ == '__main__':
    TestCase.FDSTestCase.fdsGetCmdLineConfigs(sys.argv)
    unittest.main(testRunner=xmlrunner.XMLTestRunner(output='test-reports'))
