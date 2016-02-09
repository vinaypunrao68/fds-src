#!/usr/bin/python
#
# Copyright 2016 by Formation Data Systems, Inc.
#

import unittest
import TestCase
from fdslib.TestUtils import get_node_service

class ISCSIFixture(TestCase.FDSTestCase):
    """Maps FDS volume names to iSCSI target names

    Attributes
    ----------
    _volumes : dict
        Uses volume names as unique keys

        Example:

        {'vol1': {'target_name': 'iqn.2012-05.com.formationds:vol1',
            'sd_device': '/dev/sda', 'sg_device': '/dev/sg6' },
         'vol2': {'target_name': 'iqn.2012-05.com.formationds:vol2',
            'sd_device': '/dev/sdb', 'sg_device': '/dev/sg7' }
        }
    """
    # By convention, leading underscore means 'implementation detail'.
    # Please don't use outside this module.
    _volumes = {}

    def __init__(self, parameters=None, testCaseName=None, testCaseDriver=None,
            testCaseDescription=None, testCaseAlwaysExecute=False, fork=False):
        super(ISCSIFixture, self).__init__(parameters,
                testCaseName,
                testCaseDriver,
                testCaseDescription,
                testCaseAlwaysExecute,
                fork)

    def setUp(self, parameters=None):
        super(ISCSIFixture, self).setUp(parameters)

    def tearDown(self):
        super(ISCSIFixture, self).tearDown()

    def addVolumeName(self, volume_name):
        """
        Parameters
        ----------
        volume_name : str
            FDS volume name to add to mapping
        """
        if not volume_name in ISCSIFixture._volumes:
            ISCSIFixture._volumes[volume_name] = {}


    def getDriveDevice(self, volume_name):
        """Get drive device mapped to given volume name

        Parameters
        ----------
        volume_name : str
            FDS volume name

        Returns
        -------
        str
            Drive device name like '/dev/sdb'
        """
        self.validateVolumeName(volume_name)
        if not volume_name in ISCSIFixture._volumes:
            self.log.error("Unknown volume %s" % volume_name)
            raise Exception

        d = ISCSIFixture._volumes[volume_name]
        if not 'sd_device' in d:
            return None
        return d['sd_device']


    def getGenericDevice(self, volume_name):
        """Get generic, char based pass-through device mapped to given volume name

        Parameters
        ----------
        volume_name : str
            FDS volume name

        Returns
        -------
        str
            Generic device name like '/dev/sg2'
        """
        self.validateVolumeName(volume_name)
        if not volume_name in ISCSIFixture._volumes:
            self.log.error("Unknown volume %s" % volume_name)
            raise Exception

        d = ISCSIFixture._volumes[volume_name]
        if not 'sg_device' in d:
            return None
        return d['sg_device']


    def getTargetName(self, volume_name):
        """Get iSCSI node name mapped to given volume name

        Parameters
        ----------
        volume_name : str
            FDS volume name used to look-up iSCSI node name

        Returns
        -------
        str
            iSCSI node name like 'iqn.2012-05.com.formationds:vol1'
        """
        self.validateVolumeName(volume_name)
        if not volume_name in ISCSIFixture._volumes:
            self.log.error("Unknown volume %s" % volume_name)
            raise Exception

        d = ISCSIFixture._volumes[volume_name]
        if not 'target_name' in d:
            return None
        return d['target_name']


    def setDriveDevice(self, sd_device, volume_name):
        """
        Parameters
        ----------
        sd_device : str
            Device name like '/dev/sdb', for use with block driver
        volume_name : str
            The FDS volume name (serves as key)
        """
        self.validateVolumeName(volume_name)
        if not volume_name in ISCSIFixture._volumes:
            ISCSIFixture._volumes[volume_name] = {}
        d = ISCSIFixture._volumes[volume_name]
        d['sd_device'] = sd_device


    def setGenericDevice(self, sg_device, volume_name):
        """
        Parameters
        ----------
        sg_device : str
            Device name like '/dev/sg2', for use with block driver
        volume_name : str
            The FDS volume name (serves as key)
        """
        self.validateVolumeName(volume_name)
        if not volume_name in ISCSIFixture._volumes:
            ISCSIFixture._volumes[volume_name] = {}
        d = ISCSIFixture._volumes[volume_name]
        d['sg_device'] = sg_device


    def setTargetName(self, target_name, volume_name):
        """
        Parameters
        ----------
        target_name : str
            The iSCSI node name, following iqn convention
        volume_name : str
            The FDS volume name (serves as key)
        """
        self.validateVolumeName(volume_name)
        if not volume_name in ISCSIFixture._volumes:
            ISCSIFixture._volumes[volume_name] = {}
        d = ISCSIFixture._volumes[volume_name]
        d['target_name'] = target_name


    def validateVolumeName(self, volume_name):

        if not volume_name:
            self.log.error("Missing required argument")
            raise Exception
        if not isinstance(volume_name, str):
            self.log.error("Invalid argument")
            raise Exception


    def getAMEndpoint(self, om_node):
        """
        Returns
        -------
        str
            IP Address for an AM node
        """
        if not om_node:
            self.log.error("OM node required")
            raise Exception

        # Uses CLI to identify an AM node. The AM node can present iSCSI targets.
        node_service = get_node_service(self,om_node.nd_conf_dict['ip'])
        nodes = node_service.list_nodes()
        am_node = None
        for n in nodes:
            am_state = n.services['AM'][0].status.state
            if am_state == 'RUNNING':
                am_node = n
                break;
        if not am_node:
            self.log.error('AM service not found')
            raise Exception

        return am_node.address.ipv4address


    def getInitiatorEndpoint(self, initiator_name, fdscfg):
        """
        Parameters
        ----------
        initiator_name : str
        fdscfg : FdsConfigRun

        Returns
        -------
        str
            IP Address for the named initiator
        """
        if not initiator_name:
            self.log.error("Initiator name required")
            raise Exception

        initiator_ip = None
        initiators = fdscfg.rt_obj.cfg_initiators
        for i in initiators:
            if i.nd_conf_dict['node-name'] == initiator_name:
                initiator_ip = i.nd_conf_dict['ip']
                break;

        if not initiator_ip:
            self.log.error('Initiator {0} not found in FdsConfigFile.'.format(initiator_name))
            raise Exception

        return initiator_ip

