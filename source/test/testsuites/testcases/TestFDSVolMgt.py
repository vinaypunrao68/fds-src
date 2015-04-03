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

# Module-specific requirements
import sys
import os


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

            cmd = (' volume create %s' % volume.nd_conf_dict['vol-name'])

            if 'id' not in volume.nd_conf_dict:
                raise Exception('Volume section %s must have "id" keyword.' % volume.nd_conf_dict['vol-name'])
            cmd = cmd + (' --tenant-id %s' % volume.nd_conf_dict['id'])

            if 'access' not in volume.nd_conf_dict:
                access = 'object'
            else:
                access = volume.nd_conf_dict['access']

            # Size only makes sense for block volumes
            if 'block' == access:
                if 'size' not in volume.nd_conf_dict:
                    raise Exception('Volume section %s must have "size" keyword.' % volume.nd_conf_dict['vol-name'])
                cmd = cmd + (' --blk-dev-size %s' % volume.nd_conf_dict['size'])

            cmd = cmd + (' --vol-type %s' % access)

            if 'media' not in volume.nd_conf_dict:
                media = 'hdd'
            else:
                media = volume.nd_conf_dict['media']

            cmd = cmd + (' --media-policy %s' % media)

            self.log.info("Create volume %s on OM node %s." %
                          (volume.nd_conf_dict['vol-name'], om_node.nd_conf_dict['node-name']))

            status = om_node.nd_agent.exec_wait('bash -c \"(./fdsconsole.py %s > %s/cli.out 2>&1 &) \"' %
                                                (cmd, log_dir if om_node.nd_agent.env_install else "."),
                                                fds_tools=True)

            if status != 0:
                self.log.error("Volume %s creation on %s returned status %d." %
                               (volume.nd_conf_dict['vol-name'], om_node.nd_conf_dict['node-name'], status))
                return False
            elif self.passedVolume is not None:
                break

        return True


# This class contains the attributes and methods to test
# volume attachment.
class TestVolumeAttach(TestCase.FDSTestCase):
    def __init__(self, parameters=None, volume=None):
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_VolumeAttach,
                                             "Attach volume")

        self.passedVolume = volume

    def test_VolumeAttach(self):
        """
        Test Case:
        Attempt to attach a volume.
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]

        # Currently, all volumes are attached using our one well-known OM.
        om_node = fdscfg.rt_om_node
        log_dir = fdscfg.rt_env.get_log_dir()

        volumes = fdscfg.rt_get_obj('cfg_volumes')
        for volume in volumes:
            # If we were passed a volume, attach that one and exit.
            if self.passedVolume is not None:
                volume = self.passedVolume


            # Object volumes currently attach implicitly
            if 'block' != volume.nd_conf_dict['access']:
                break

            cmd = (' attach %s %s' %
               (volume.nd_conf_dict['vol-name'],
                volume.nd_am_node.nd_conf_dict['node-name']))

            self.log.info("Attach volume %s on OM node %s." %
                          (volume.nd_conf_dict['vol-name'], om_node.nd_conf_dict['node-name']))

            status = om_node.nd_agent.exec_wait('bash -c \"(nohup ./nbdadm.py  %s > %s/nbdadm.out 2>&1 &) \"' %
                                                (cmd, log_dir if om_node.nd_agent.env_install else "."),
                                                fds_tools=True)

            if status != 0:
                self.log.error("Attach volume %s on %s returned status %d." %
                               (volume.nd_conf_dict['vol-name'], om_node.nd_conf_dict['node-name'], status))
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
        Attempt to attach a volume.
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]

        # Currently, all volumes are attached using our one well-known OM.
        om_node = fdscfg.rt_om_node
        log_dir = fdscfg.rt_env.get_log_dir()

        volumes = fdscfg.rt_get_obj('cfg_volumes')
        for volume in volumes:
            # If we were passed a volume, attach that one and exit.
            if self.passedVolume is not None:
                volume = self.passedVolume

            cmd = (' volume delete %s' %
               (volume.nd_conf_dict['vol-name']))

            self.log.info("Delete volume %s on OM node %s." %
                          (volume.nd_conf_dict['vol-name'], om_node.nd_conf_dict['node-name']))

            status = om_node.nd_agent.exec_wait('bash -c \"(./fdsconsole.py %s > %s/cli.out 2>&1) \"' %
                                                (cmd, log_dir if om_node.nd_agent.env_install else "."),
                                                fds_tools=True)

            if status != 0:
                self.log.error("Delete volume %s on %s returned status %d." %
                               (volume.nd_conf_dict['vol-name'], om_node.nd_conf_dict['node-name'], status))
                return False
            elif self.passedVolume is not None:
                break

        return True


if __name__ == '__main__':
    TestCase.FDSTestCase.fdsGetCmdLineConfigs(sys.argv)
    unittest.main()
