#!/usr/bin/python
#
# Copyright 2016 by Formation Data Systems, Inc.
#

import unittest
import TestCase

class ISCSIFixture(TestCase.FDSTestCase):
    """Maps FDS volume names to iSCSI target names

    Attributes
    ----------
    _volumes : dict
        Uses volume names as unique keys

        Example:

        {'vol1': {'target_name': 'iqn.2012-05.com.formationds:vol1',
            'sd_device': '/dev/sd4', 'sg_device': '/dev/sg6' },
         'vol2': {'target_name': 'iqn.2012-05.com.formationds:vol2',
            'sd_device': '/dev/sd5', 'sg_device': '/dev/sg7' }
        }
    """
    # By convention, leading underscore means 'implementation detail'.
    # Please don't use outside this module.
    _volumes = {}

    _volume_name = "volISCSI"
    _target_name = None

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

#    def getVolumeName(self):
#        return ISCSIFixture._volume_name

    def addVolumeName(self, volume_name):
        """
        Parameters
        ----------
        volume_name : str
            FDS volume name to add to mapping
        """
        if not volume_name in ISCSIFixture._volumes:
            ISCSIFixture._volumes[volume_name] = {}
#        ISCSIFixture._volume_name = volume_name


    def getDriveDevice(self, volume_name):
        """Get drive device mapped to given volume name

        Parameters
        ----------
        volume_name : str
            FDS volume name

        Returns
        -------
        str
            Drive device name like '/dev/sd2'
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
#        return ISCSIFixture._target_name


    def setDriveDevice(self, sd_device, volume_name):
        """
        Parameters
        ----------
        sd_device : str
            Device name like '/dev/sd2', for use with block driver
        volume_name : str
            The FDS volume name (serves as key)
        """
        validateVolumeName(volume_name)
        if not volume_name in ISCSIFixture._volumes:
            ISCSIFixture._volumes[volume_name] = {}
        d = ISCSIFixture._volumes[volume_name]
        d['sd_device'] = sd_device


    def setGenericDevice(self, sg_device, volume_name):
        """
        Parameters
        ----------
        sd_device : str
            Device name like '/dev/sd2', for use with block driver
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
        # TODO: use a dictionary here
        #ISCSIFixture._target_name=target_name


    def validateVolumeName(self, volume_name):

        if not volume_name:
            self.log.error("Missing required argument")
            raise Exception
        if not isinstance(volume_name, str):
            self.log.error("Invalid argument")
            raise Exception

