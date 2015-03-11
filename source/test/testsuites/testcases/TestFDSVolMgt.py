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
        fds_dir = om_node.nd_conf_dict['fds_root']
        bin_dir = fdscfg.rt_env.get_bin_dir(debug=False)
        log_dir = fdscfg.rt_env.get_log_dir()

        volumes = fdscfg.rt_get_obj('cfg_volumes')
        for volume in volumes:
            # If we were passed a volume, create that one and exit.
            if self.passedVolume is not None:
                volume = self.passedVolume

            cmd = (' --volume-create %s' % volume.nd_conf_dict['vol-name'])

            if 'id' not in volume.nd_conf_dict:
                raise Exception('Volume section %s must have "id" keyword.' % volume.nd_conf_dict['vol-name'])
            cmd = cmd + (' -i %s' % volume.nd_conf_dict['id'])

            if 'size' not in volume.nd_conf_dict:
                raise Exception('Volume section %s must have "size" keyword.' % volume.nd_conf_dict['vol-name'])
            cmd = cmd + (' -s %s' % volume.nd_conf_dict['size'])

            if 'policy' not in volume.nd_conf_dict:
                raise Exception('Volume section %s must have "policy" keyword.' % volume.nd_conf_dict['vol-name'])
            cmd = cmd + (' -p %s' % volume.nd_conf_dict['policy'])

            if 'access' not in volume.nd_conf_dict:
                access = 's3'
            else:
                access = volume.nd_conf_dict['access']

            cmd = cmd + (' -y %s' % access)

            self.log.info("Create volume %s on OM node %s." %
                          (volume.nd_conf_dict['vol-name'], om_node.nd_conf_dict['node-name']))

            cur_dir = os.getcwd()
            os.chdir(bin_dir)

            status = om_node.nd_agent.exec_wait('bash -c \"(nohup ./fdscli --fds-root %s %s > '
                                          '%s/cli.out 2>&1 &) \"' %
                                          (fds_dir, cmd, log_dir if om_node.nd_agent.env_install else "."))

            os.chdir(cur_dir)

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
        fds_dir = om_node.nd_conf_dict['fds_root']
        bin_dir = fdscfg.rt_env.get_bin_dir(debug=False)
        log_dir = fdscfg.rt_env.get_log_dir()

        volumes = fdscfg.rt_get_obj('cfg_volumes')
        for volume in volumes:
            # If we were passed a volume, attach that one and exit.
            if self.passedVolume is not None:
                volume = self.passedVolume

            cmd = (' --volume-attach %s -i %s -n %s' %
               (volume.nd_conf_dict['vol-name'],
                volume.nd_conf_dict['id'],
                volume.nd_am_node.nd_conf_dict['node-name']))

            self.log.info("Attach volume %s on OM node %s." %
                          (volume.nd_conf_dict['vol-name'], om_node.nd_conf_dict['node-name']))

            cur_dir = os.getcwd()
            os.chdir(bin_dir)

            status = om_node.nd_agent.exec_wait('bash -c \"(nohup ./fdscli --fds-root %s %s > '
                                          '%s/cli.out 2>&1 &) \"' %
                                          (fds_dir, cmd, log_dir if om_node.nd_agent.env_install else "."))

            os.chdir(cur_dir)

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
        fds_dir = om_node.nd_conf_dict['fds_root']
        bin_dir = fdscfg.rt_env.get_bin_dir(debug=False)
        log_dir = fdscfg.rt_env.get_log_dir()

        volumes = fdscfg.rt_get_obj('cfg_volumes')
        for volume in volumes:
            # If we were passed a volume, attach that one and exit.
            if self.passedVolume is not None:
                volume = self.passedVolume

            cmd = (' --volume-delete %s -i %s' %
               (volume.nd_conf_dict['vol-name'],
                volume.nd_conf_dict['id']))

            self.log.info("Delete volume %s on OM node %s." %
                          (volume.nd_conf_dict['vol-name'], om_node.nd_conf_dict['node-name']))

            cur_dir = os.getcwd()
            os.chdir(bin_dir)

            status = om_node.nd_agent.exec_wait('bash -c \"(./fdscli --fds-root %s %s > '
                                          '%s/cli.out 2>&1) \"' %
                                          (fds_dir, cmd, log_dir if om_node.nd_agent.env_install else "."))

            os.chdir(cur_dir)

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