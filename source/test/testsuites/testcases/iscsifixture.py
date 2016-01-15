#!/usr/bin/python
#
# Copyright 2016 by Formation Data Systems, Inc.
#

import unittest
import TestCase

class ISCSIFixture(TestCase.FDSTestCase):
    """Maintains mapping from volume name to ISCSI target name

    TODO: change from a single mapping to a multi-map (dictionary)
    Example:

    {'volume_name': 'volISCSI1', 'target_name':'iqn.2012-05.com.formationds:.volISCSI1', 
     'sg_device': '/dev/sg1', 'sd_device': '/dev/sd1'}
    """
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

    def getVolumeName(self):
        return ISCSIFixture._volume_name

    def setVolumeName(self, volume_name):
        """
        Parameters
        ----------
        volume_name : str
            FDS volume name to add to mapping
        """
        ISCSIFixture._volume_name = volume_name

    def getTargetName(self, volume_name):
        """
        Parameters
        ----------
        volume_name : str
            FDS volume name used to look-up iSCSI node name
        """
        # TODO: use a dictionary for look-up
        return ISCSIFixture._target_name

    def setTargetName(self, target_name, volume_name):
        """
        Parameters
        ----------
        target_name : str
            The iSCSI node name, following iqn convention
        volume_name : str
            The FDS volume name (serves as key)
        """
        # TODO: use a dictionary here
        ISCSIFixture._target_name=target_name

