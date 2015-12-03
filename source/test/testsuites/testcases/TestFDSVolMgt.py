#!/usr/bin/python
#
# Copyright 2015 by Formation Data Systems, Inc.
#
# Test cases associated with volume management.
#

# FDS test-case pattern requirements.
import unittest
import traceback
import TestCase
from fdslib.TestUtils import findNodeFromInv
from fdslib.TestUtils import check_localhost
# Module-specific requirements
import sys
import os
import socket
from fdscli.services.volume_service import VolumeService
from fdscli.services.fds_auth import *
from fdslib.TestUtils import create_fdsConf_file
from fdslib.TestUtils import convertor
from fdslib.TestUtils import get_volume_service
from fdscli.model.fds_error import FdsError

# This class contains the attributes and methods to test
# volume creation.
class TestVolumeCreate(TestCase.FDSTestCase):
    def __init__(self, parameters=None, volume=None):
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_VolumeCreate,
                                             "Volume creation")

        self.passedVolume = volume

    def test_VolumeCreate(self):
        """
        Test Case:
        Attempt to create a volume.
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]

        # Currently, all volumes are created using our one well-known OM.
        om_node = fdscfg.rt_om_node
        log_dir = fdscfg.rt_env.get_log_dir()

        volumes = fdscfg.rt_get_obj('cfg_volumes')
        for volume in volumes:
            # If we were passed a volume, create that one and exit.
            if self.passedVolume is not None:
                volume = self.passedVolume
            if 'id' not in volume.nd_conf_dict:
                raise Exception('Volume section %s must have "id" keyword.' % volume.nd_conf_dict['vol-name'])

            self.log.info("Create volume %s on OM node %s." %
                          (volume.nd_conf_dict['vol-name'], om_node.nd_conf_dict['node-name']))

            vol_service = get_volume_service(self,om_node.nd_conf_dict['ip'])
            newVolume = convertor(volume, fdscfg)
            status = vol_service.create_volume(newVolume)

            if isinstance(status, FdsError):
                self.log.error("Volume %s creation on %s returned status %s." %
                               (volume.nd_conf_dict['vol-name'], om_node.nd_conf_dict['node-name'], status))
                return False
            elif self.passedVolume is not None:
                break

        return True


# This class contains the attributes and methods to test
# volume attachment.
class TestVolumeAttach(TestCase.FDSTestCase):
    def __init__(self, parameters=None, volume=None, node=None, expect_to_fail=False):
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_VolumeAttach,
                                             "Attach volume")

        self.passedVolume = volume
        self.passedNode = node
        self.expect_to_fail = expect_to_fail

    def test_VolumeAttach(self):
        """
        Test Case:
        Attempt to attach a volume.
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]

        # Currently, all volumes are attached using our one well-known OM
        # in case a node hosting an AM is not passed as an argument.
        om_node = fdscfg.rt_om_node
        log_dir = fdscfg.rt_env.get_log_dir()

        volumes = fdscfg.rt_get_obj('cfg_volumes')
        for volume in volumes:
            # If we were passed a volume, attach that one and exit.
            if self.passedVolume is not None:
                volume = self.passedVolume

            # If a node is passed, attach volume to AM on this node.
            if self.passedNode is not None:
                am_node = self.passedNode
            else:
                am_node = volume.nd_am_node.nd_conf_dict['node-name']

            volName = volume.nd_conf_dict['vol-name']

            # Object volumes currently attach implicitly
            if 'block' != volume.nd_conf_dict['access']:
                break

            nodes = fdscfg.rt_obj.cfg_nodes
            node = findNodeFromInv(nodes, am_node)

            ip = node.nd_conf_dict['ip']
            offset = 3809
            port = int(node.nd_conf_dict['fds_port']) + offset
            self.log.info("Attach volume %s on node %s." % (volName, am_node))
            cmd = ('attach %s:%s %s' % (ip, port, volName))
            if check_localhost(ip):
                cinder_dir = os.path.join(fdscfg.rt_env.get_fds_source(), 'cinder')
            else:
                cinder_dir= os.path.join('/fds/sbin')
            status, stdout = om_node.nd_agent.exec_wait('bash -c \"(nohup %s/nbdadm.py  %s) \"' %
                                                        (cinder_dir, cmd), return_stdin=True)
            if (status != 0) != self.expect_to_fail:
                self.log.error("Attach volume %s on %s returned status %d." %
                               (volName, am_node, status))
                return False

            #stdout has the nbd device to which the volume was attached
            volume.nd_conf_dict['nbd-dev'] = stdout
            self.log.info("volume = %s and it's nbd device = %s" %
                          (volume.nd_conf_dict['vol-name'], volume.nd_conf_dict['nbd-dev']))

            if self.passedVolume is not None:
                break

        return True

# This class contains the attributes and methods to test
# volume detachment.
class TestVolumeDetach(TestCase.FDSTestCase):
    def __init__(self, parameters=None, volume=None, node=None):
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_VolumeDetach,
                                             "Detach volume")

        self.passedVolume = volume
        self.passedNode = node

    def test_VolumeDetach(self):
        """
        Test Case:
        Attempt to detach a volume.
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]

        # Currently, all volumes are assumed to be attached to one well-known
        # OM in case a node hosting an AM is not passed as an argument.
        om_node = fdscfg.rt_om_node
        log_dir = fdscfg.rt_env.get_log_dir()

        volumes = fdscfg.rt_get_obj('cfg_volumes')
        for volume in volumes:
            # If we were passed a volume, detach that one and exit.
            if self.passedVolume is not None:
                volume = self.passedVolume

            if self.passedNode is not None:
                am_node = self.passedNode
            else:
                am_node = volume.nd_am_node.nd_conf_dict['node-name']

            volName = volume.nd_conf_dict['vol-name']

            # Object volumes currently detach implicitly
            if 'block' != volume.nd_conf_dict['access']:
                break

            nodes = fdscfg.rt_obj.cfg_nodes
            for node in nodes:
                if am_node == node.nd_conf_dict['node-name']:
                    break;
            ip = node.nd_conf_dict['ip']
            if not check_localhost(ip):
                log_dir = '/tmp'

            offset = 3809
            port = int(node.nd_conf_dict['fds_port']) + offset
            self.log.info("Detach volume %s on node %s." % (volName, am_node))
            cmd = ('detach %s' % (volName))

            if check_localhost(ip):
                cinder_dir = os.path.join(fdscfg.rt_env.get_fds_source(), 'source/cinder')
            else:
                cinder_dir= os.path.join('/fds/sbin')

            status = om_node.nd_agent.exec_wait('bash -c \"(nohup %s/nbdadm.py  %s >> %s/nbdadm.out 2>&1 &) \"' %
                                                (cinder_dir, cmd, log_dir if om_node.nd_agent.env_install else "."),
                                                fds_tools=True)

            if status != 0:
                self.log.error("Detach volume %s on %s returned status %d." %
                               (volName, am_node, status))
                return False
            elif self.passedVolume is not None:
                break

        return True

# This class contains the attributes and methods to test
# volume delete.
class TestVolumeDelete(TestCase.FDSTestCase):
    def __init__(self, parameters=None, volume=None):
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_VolumeDelete,
                                             "Delete volume")

        self.passedVolume = volume


    def test_VolumeDelete(self):
        """
        Test Case:
        Attempt to delete a volume.
        """
        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]
        om_node = fdscfg.rt_om_node

        vol_service = get_volume_service(self,om_node.nd_conf_dict['ip'])
        volumes = vol_service.list_volumes()
        for volume in volumes:
        # We arent comapring with ID because volume id is not the same as give in cfg vol definition, new REST API assigns new vol_id
            if self.passedVolume.nd_conf_dict['vol-name'] == volume.name:
                volume_id = volume.id
                break
            else:
                continue

        if volume_id is None:
            self.log.error("Could not find volume %s"%(self.passedVolume.nd_conf_dict['vol-name']))
            return False

        self.log.info("Delete volume %s on OM node %s." %
                          (volume.name, om_node.nd_conf_dict['node-name']))
        vol_service = get_volume_service(self,om_node.nd_conf_dict['ip'])
        status = vol_service.delete_volume(volume_id)

        if isinstance(status, FdsError):
            self.log.error("Delete volume %s on %s returned status as %s." %
                               (volume.name, om_node.nd_conf_dict['node-name'], status))
            return False

        return True


if __name__ == '__main__':
    TestCase.FDSTestCase.fdsGetCmdLineConfigs(sys.argv)
    unittest.main()
