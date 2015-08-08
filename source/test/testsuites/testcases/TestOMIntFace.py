#!/usr/bin/python
#
# Copyright 2014 by Formation Data Systems, Inc.
#

# FDS test-case pattern requirements.
import unittest
import traceback
import TestCase

# Module-specific requirements
import sys
import time
import fdslib.restendpoint as rep


# This class contains the attributes and methods to test
# the Orchestration Manager interface to retrieve an authorization token
# for use with an S3 connector in establishing a connection, for example.
class TestGetAuthToken(TestCase.FDSTestCase):
    def __init__(self, parameters=None):
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_GetAuthToken,
                                             "Retrieving an authorization token")

    def test_GetAuthToken(self):
        """
        Test Case:
        Attempt to retrieve an authorization token from OM.
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]
        om_node = fdscfg.rt_om_node

        self.log.info("Ask OM on %s for an authorization token." % om_node.nd_conf_dict['node-name'])
 
        count = 0
        while count < 10:
           # With "auth=False", the "connection string" is set up but not tested.
           omREP = rep.RestEndpoint(om_node.nd_conf_dict['ip'], port=7777, user='admin', password='admin', auth=False,
                                 ssl=False)

           # Despite the generic nature of its name, this method will actually log
           # into the specified OM, retrieve and return the authorization token.
           #
           # We'll capture the token among our OM configuration so that it can be used
           # in subsequent testing.
           om_node.auth_token = omREP.login(user='admin', password='admin')

           if not om_node.auth_token:
               self.log.info("OM on %s still isn't available, retry %d." % (om_node.nd_conf_dict['node-name'], count))
               count = count + 1
               time.sleep(5)
           else:
               return True

        if not om_node.auth_token:
            self.log.error("OM on %s did not return an authorization token." % om_node.nd_conf_dict['node-name'])
            return False

        return True


if __name__ == '__main__':
    TestCase.FDSTestCase.fdsGetCmdLineConfigs(sys.argv)
    unittest.main()
