#!/usr/bin/python
#
# Copyright 2014 by Formation Data Systems, Inc.
#

"""
Module to wrap the usage of smchk for verifying sm tokens and metadata within system tests
"""

import TestCase
import sys
import os
import subprocess
import re

testlib_path = os.path.join(os.getcwd(), '..',)
sys.path.append(testlib_path)
print("SYSPATH = {}".format(sys.path))
from testlib.sm import dlt

class TestVerifyMigrations(TestCase.FDSTestCase):
    """
    Run a test that verifies target dlt tokens
    """
    def __init__(self, parameters=None):
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_SmChk,
                                             "targeted smchk for token and sm metadata validation")

        self.parameters = parameters


    def test_SmChk(self):
        """
        Verify all nodes have the same tokens and that those tokens are correct
        """

        # Get the token distribution layout by calling smchk --verbose
        fdscfg = self.parameters["fdscfg"]
        om_node = fdscfg.rt_om_node

        dlt_file = os.path.join(om_node.nd_conf_dict['fds-root'], 'var', 'logs', 'currentDLT')
        print dlt_file

        current_dlt = dlt.load_dlt(dlt_file)

        print("CURRENT DLT = {}".format(current_dlt))

        # for col in current_dlt:


        # Call fdsconsole to check tokens
        # call = ['fdsconsole.py', 'smdebug', 'startSmchk', node_uuid, '--targetTokens', target_tokens]


        # For each token group, check the tokens of each group

        # Compare the output of each