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
import time
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

    def _get_latest_sm_log(self, node):
        """
        Grabs the latest SM log from fds_root/var/logs/ directory
        :param node: Node object to find the sm log for
        :return: Full path to the latest SM log file
        """
        fds_dir = node.nd_conf_dict['fds_root']
        # Get a list of all log files.
        log_files = os.listdir(fds_dir + "/var/logs")
        # Make the full path for each log
        log_files = map(lambda x: os.path.join(fds_dir, 'var', 'logs', x), log_files)
        # Find just the SM logs
        log_files = filter(lambda x: 'sm' in x, log_files)
        # Now get just the latest log by sorting on mtime and taking the last one
        latest_log = sorted(log_files, key=os.path.getmtime)[-1]

        return latest_log


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
            self.log.info("Node: {} => Tokens: {}".format(hex(node_uuid), tokens))

        for node_uuid, tokens in current_dlt.items():
            # Setup stuff we'll want to check against
            num_checked_tokens = len(tokens)

            tokens_str = str(tokens)[1:-1]
            tokens_str = ''.join(tokens_str.split())
            self.log.info("Starting sm check for node {} with num_tokens = {}".format(hex(node_uuid), num_checked_tokens))
            call = ' '.join(['./fdsconsole.py', 'smdebug', 'startSmchk', str(node_uuid), tokens_str])
            res = om_node.nd_agent.exec_wait(call, fds_tools=True)
            if res != 0:
                self.log.error("start smchk failed...")
                return False

            # We'll want to search the SM log for this output:
            # Completed SM Integrity Check:
            # TODO(brian): We ***really*** need a better way of doing this...
            # Right now we've essentially got to find all of the SM logs, open the most recent and then regex search
            # backwards to find the most recent smchk entry.
            # TODO(brian): This will not work in a 'real' environment, only in simulated local env
            nodes = fdscfg.rt_obj.cfg_nodes
            for node in nodes:
                # Remember to subtract 1 from node_uuid because it's actually an SM svc uuid
                if node.nd_uuid is not None and (int(node.nd_uuid, 0) == node_uuid - 1):
                    latest_log = self._get_latest_sm_log(node)
                    self.log.info("Searching in log file {} for node {}".format(latest_log, node.nd_uuid))
                    fh = open(latest_log, 'r')
                    # Just keep looking until we find the log message
                    result = []
                    timeout_cntr = 0 # 100 iterations of find/sleep will be ~30 minutes
                    while not result:
                        # Set the file's cursor back to 0
                        fh.seek(0)
                        result = smchk_re.findall(fh.read())
                        # For now, just sleep for 20 second intervals
                        # TODO: Replace this with a smarter mechanism
                        if timeout_cntr == 100:
                            break
                        time.sleep(20)
                        timeout_cntr += 1

                    fh.close()
                    if len(result) == 0 or (result[-1][2]!= num_checked_tokens and
                                                 result[-1][3] == 0 and
                                                 result[-1][4] == 0):
                        return False
                    else:
                        self.log.info("Completed SM Integrity Check for node {}: totalNumTokens={}"
                                      "totalNumTokensVerified={} numCorrupted={} numOwnershipMismatches={}"
                                      .format(node.nd_uuid, result[-1][1], result[-1][2], result[-1][3], result[-1][4]))
        return True
