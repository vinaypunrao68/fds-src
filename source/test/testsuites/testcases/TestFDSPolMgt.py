#!/usr/bin/python
#
# Copyright 2015 by Formation Data Systems, Inc.
#
# Test cases associated with volume policy management.
#

# FDS test-case pattern requirements.
import unittest
import traceback
import TestCase

# Module-specific requirements
import sys
import os


# This class contains the attributes and methods to test
# volume policy creation.
class TestPolicyCreate(TestCase.FDSTestCase):
    def __init__(self, parameters=None, policy=None):
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_PolicyCreate,
                                             "Policy creation")

        self.passedPolicy = policy

    def test_PolicyCreate(self):
        """
        Test Case:
        Attempt to create a policy.
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]

        # Currently, all policies are created using our one well-known OM.
        om_node = fdscfg.rt_om_node
        log_dir = fdscfg.rt_env.get_log_dir()

        policies = fdscfg.rt_get_obj('cfg_vol_pol')
        for policy in policies:
            # If we were passed a policy, create that one and exit.
            if self.passedPolicy is not None:
                policy = self.passedPolicy

            iops_min = 0
            if 'iops_min' in policy.nd_conf_dict:
                iops_min = policy.nd_conf_dict['iops_min']

            iops_max = 0
            if 'iops_max' in policy.nd_conf_dict:
                iops_max = policy.nd_conf_dict['iops_max']

            priority = 0
            if 'priority' in policy.nd_conf_dict:
                priority = policy.nd_conf_dict['priority']

            self.log.info("Create a policy %s on OM node %s." %
                          (policy.nd_conf_dict['pol-name'], om_node.nd_conf_dict['node-name']))

            status = om_node.nd_agent.exec_wait('bash -c \"(./fdsconsole.py qospolicy create {} {} {} {} > '
                                                '{}/fdsconsole.out 2>&1) \"'.format(policy.nd_conf_dict['pol-name'],
                                                                                    iops_min,
                                                                                    iops_max,
                                                                                    priority,
                                                                                    log_dir),
                                                fds_tools=True)

            if status != 0:
                self.log.error("Policy %s creation on %s returned status %d." %
                               (policy.nd_conf_dict['pol-name'], om_node.nd_conf_dict['node-name'], status))
                return False
            elif self.passedPolicy is not None:
                break

        return True


# This class contains the attributes and methods to test
# volume policy deletion.
class TestPolicyDelete(TestCase.FDSTestCase):
    def __init__(self, parameters=None, policy=None):
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_PolicyDelete,
                                             "Policy deletion")

        self.passedPolicy = policy

    def test_PolicyDelete(self):
        """
        Test Case:
        Attempt to delete a policy.
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]

        # Currently, all policies are created using our one well-known OM.
        om_node = fdscfg.rt_om_node
        log_dir = fdscfg.rt_env.get_log_dir()

        policies = fdscfg.rt_get_obj('cfg_vol_pol')
        for policy in policies:
            # If we were passed a policy, create that one and exit.
            if self.passedPolicy is not None:
                policy = self.passedPolicy

            self.log.info("Delete a policy %s on OM node %s." %
                          (policy.nd_conf_dict['pol-name'], om_node.nd_conf_dict['node-name']))

            status = om_node.nd_agent.exec_wait('bash -c \"(./fdsconsole.py qospolicy delete {} > '
                                                '{}/fdsconsole.out 2>&1) \"'.format(policy.nd_conf_dict['pol-name'],
                                                                                    log_dir),
                                                fds_tools=True)

            if status != 0:
                self.log.error("Policy %s deletion on %s returned status %d." %
                               (policy.nd_conf_dict['pol-name'], om_node.nd_conf_dict['node-name'], status))
                return False
            elif self.passedPolicy is not None:
                break

        return True


if __name__ == '__main__':
    TestCase.FDSTestCase.fdsGetCmdLineConfigs(sys.argv)
    unittest.main()