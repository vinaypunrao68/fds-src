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
from testlib.sm.dlt import DLT as dlt

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

        smchk_re = re.compile(r'- Completed SM Integrity Check: active=(?P<is_active>true|false) '
                              r'totalNumTokens=(?P<total_tokens>\d+) totalNumTokensVerified=(?P<total_verified>\d+) '
                              r'numCorrupted=(?P<num_corrupted>\d+) '
                              r'numOwnershipMismatches=(?P<ownership_mismatches>\d+) '
                              r'numActiveObjects=(?P<active_objects>\d+)')

        fdscfg = self.parameters["fdscfg"]
        om_node = fdscfg.rt_om_node

        dlt_file = os.path.join(om_node.nd_conf_dict['fds_root'], 'var', 'logs', 'currentDLT')

        current_dlt = dlt.transpose_dlt(dlt.load_dlt(dlt_file))

        for node_uuid, tokens in current_dlt.items():

            # Setup stuff we'll want to check against
            num_checked_tokens = len(tokens)

            tokens_str = str(tokens)[1:-1]
            tokens_str = ''.join(tokens_str.split())

            call = ' '.join(['./fdsconsole.py', 'smdebug', 'startSmchk', str(node_uuid), '--targetTokens', tokens_str])
            om_node.nd_agent.exec_wait(call, fds_tools=True)

            # We'll want to search the SM log for this output:
            # Completed SM Integrity Check:
            # TODO(brian): We ***really*** need a better way of doing this...
            # Right now we've essentially got to find all of the SM logs, open the most recent and then regex search
            # backwards to find the most recent smchk entry.
            # TODO(brian): This will not work in a 'real' environment, only in simulated local env
            nodes = fdscfg.rt_obj.cfg_nodes
            for node in nodes:
                if node.nd_uuid == node_uuid:
                    fds_dir = node.nd_conf_dict['fds_root']
                    log_files = os.listdir(fds_dir + "/var/logs")
                    for log_file in filter(lambda x: 'sm' in x, log_files):
                        result = smchk_re.findall(log_file)
                        if result is not None and (result[-1].group('total_verified') == num_checked_tokens and
                                                           result[-1].group('num_corrupted') == 0 and
                                                           result[-1].group('ownership_mismatch') == 0):
                            # Now verify that we got what was expected
                            return True

        return False