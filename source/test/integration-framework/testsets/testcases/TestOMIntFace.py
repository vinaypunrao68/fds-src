#!/usr/bin/python
#
# Copyright 2014 by Formation Data Systems, Inc.
#

# FDS test-case pattern requirements.
import unittest
import traceback
import sys

import config
import setup_module
# Module-specific requirements
import testsets.testcase as testcase
import fdslib.restendpoint as rep


# This class contains the attributes and methods to test
# the Orchestration Manager interface to retrieve an authorization token
# for use with an S3 connector in establishing a connection, for example.
class TestGetAuthToken(testcase.FDSTestCase):
    def __init__(self, parameters=None, config_file=None, om_ip_address=None):
        super(TestGetAuthToken, self).__init__(parameters=parameters,
                                               config_file=config_file,
                                               om_ip_address=om_ip_address)


    def runTest(self):
        self.test_passed = True
        self.log.info("Running Case %s." % self.__class__.__name__)
        try:
            if not self.test_GetAuthToken():
                self.test_passed = False
        except Exception as inst:
            self.log.error("Retrieving an authorization token caused exception:")
            self.log.error(traceback.format_exc())
            self.log.error(inst.message)
            self.test_passed = False

        super(self.__class__, self).reportTestCaseResult(self.test_passed)

    def test_GetAuthToken(self):
        """
        Test Case:
        Attempt to retrieve an authorization token from OM.
        """

        # Get the FdsConfigRun object for this test.
        xargs = setup_module.fdsGetCmdLineConfigs(config.CMD_CONFIG)
        fdscfg = xargs["fdscfg"]
        om_node = fdscfg.rt_om_node

        self.log.info("Ask OM on %s for an authorization token." % 
                      om_node.nd_conf_dict['node-name'])

        # With "auth=False", the "connection string" is set up but not tested.
        omREP = rep.RestEndpoint(om_node.nd_conf_dict['ip'], port=7777, 
                                 user='admin', password='admin', auth=False,
                                 ssl=False)

        # Despite the generic nature of its name, this method will actually log
        # into the specified OM, retrieve and return the authorization token.
        #
        # We'll capture the token among our OM configuration so that it can be used
        # in subsequent testing.
        om_node.auth_token = omREP.login(user='admin', password='admin')

        if not om_node.auth_token:
            self.log.error("OM on %s did not return an authorization token." % 
                           om_node.nd_conf_dict['node-name'])
            return False

        return True
