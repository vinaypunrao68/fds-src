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
        super(TestPolicyCreate, self).__init__(parameters)

        self.passedPolicy = policy


    def runTest(self):
        test_passed = True

        if TestCase.pyUnitTCFailure:
            self.log.warning("Skipping Case %s. stop-on-fail/failfast set and a previous test case has failed." %
                             self.__class__.__name__)
            return unittest.skip("stop-on-fail/failfast set and a previous test case has failed.")
        else:
            self.log.info("Running Case %s." % self.__class__.__name__)

        try:
            if not self.test_PolicyCreate():
                test_passed = False
        except Exception as inst:
            self.log.error("Policy creation caused exception:")
            self.log.error(traceback.format_exc())
            test_passed = False

        super(self.__class__, self).reportTestCaseResult(test_passed)

        # If there is any test fixture teardown to be done, do it here.

        if self.parameters["pyUnit"]:
            self.assertTrue(test_passed)
        else:
            return test_passed


    def test_PolicyCreate(self):
        """
        Test Case:
        Attempt to create a policy.
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]

        # Currently, all policies are created using our one well-known OM.
        om_node = fdscfg.rt_om_node
        fds_dir = om_node.nd_conf_dict['fds_root']
        bin_dir = fdscfg.rt_env.get_bin_dir(debug=False)
        log_dir = fdscfg.rt_env.get_log_dir()

        policies = fdscfg.rt_get_obj('cfg_vol_pol')
        for policy in policies:
            # If we were passed a policy, create that one and exit.
            if self.passedPolicy is not None:
                policy = self.passedPolicy

            if 'id' not in policy.nd_conf_dict:
                raise Exception('Policy section %s must have an id.' % policy.nd_conf_dict['pol_name'])

            pol = policy.nd_conf_dict['id']
            cmd = (' --policy-create policy_%s -p %s' % (pol, pol))

            if 'iops_min' in policy.nd_conf_dict:
                cmd = cmd + (' -g %s' % policy.nd_conf_dict['iops_min'])

            if 'iops_max' in policy.nd_conf_dict:
                cmd = cmd + (' -m %s' % policy.nd_conf_dict['iops_max'])

            if 'priority' in policy.nd_conf_dict:
                cmd = cmd + (' -r %s' % policy.nd_conf_dict['priority'])

            self.log.info("Create a policy %s on OM node %s." %
                          (policy.nd_conf_dict['pol-name'], om_node.nd_conf_dict['node-name']))

            cur_dir = os.getcwd()
            os.chdir(bin_dir)

            status = om_node.nd_agent.exec_wait('bash -c \"(nohup ./fdscli --fds-root %s %s > '
                                          '%s/cli.out 2>&1 &) \"' %
                                          (fds_dir, cmd, log_dir if om_node.nd_agent.env_install else "."))

            os.chdir(cur_dir)

            if status != 0:
                self.log.error("Policy %s creation on %s returned status %d." %
                               (policy.nd_conf_dict['pol-name'], om_node.nd_conf_dict['node-name'], status))
                return False
            elif self.passedPolicy is not None:
                break

        return True


if __name__ == '__main__':
    TestCase.FDSTestCase.fdsGetCmdLineConfigs(sys.argv)
    unittest.main()