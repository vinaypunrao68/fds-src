#!/usr/bin/python
#
# Copyright 2014 by Formation Data Systems, Inc.
#

# FDS test-case pattern requirements.
import unittest
import traceback

import xmlrunner
import TestCase


# Module-specific requirements
import sys
import time
import logging


def pidWaitParent(pid, child_count, node):
    """
    Wait for indicated process to present a specified number of children.
    """
    log = logging.getLogger('TestFDSModMgt' + '.' + 'pidWaitParent')

    maxcount = 20

    count = 0
    found = False
    status = 0
    cmd = "pgrep -c -P %s" % pid
    while (count < maxcount) and not found:
        time.sleep(1)

        status, stdout = node.nd_agent.exec_wait(cmd, return_stdin=True)

        if int(stdout) == child_count:
            found = True

        count += 1

    if status != 0:
        log.error("Wait for %d children of %s on %s returned status %d." %
                  (child_count, pid, node.nd_conf_dict['node-name'], status))
        return False

    if not found:
        return False

    return True

def modWait(mod, node):
    """
    Wait for the named module to become active.
    """
    log = logging.getLogger('TestFDSModMgt' + '.' + 'modWait')

    maxcount = 20

    orchMgrPID = ''
    AMAgentPID = ''
    count = 0
    found = False
    status = 0
    cmd = "pgrep %s" % mod
    while (count < maxcount) and not found:
        time.sleep(1)

        status, stdout = node.nd_agent.exec_wait(cmd, return_stdin=True)

        if len(stdout) > 0:
            # For module orchMgr, there should be one child process.
            if mod == "orchMgr":
                orchMgrPID = stdout

            # For module AMAgent, there shoudl be two child processes.
            if mod == "AMAgent":
                AMAgentPID = stdout

            found = True

        count += 1

    if status != 0:
        log.error("Wait for %s on %s returned status %d." % (mod, node.nd_conf_dict['node-name'], status))
        return False

    if not found:
        return False

    if orchMgrPID != '':
        return pidWaitParent(int(orchMgrPID), 1, node)

    if AMAgentPID != '':
        return pidWaitParent(int(AMAgentPID), 2, node)

    return True


# This class contains the attributes and methods to test
# bringing up a Data Manager (DM) module.
class TestDMBringUp(TestCase.FDSTestCase):
    def __init__(self, parameters=None):
        """
        When run by a qaautotest module test runner,
        "parameters" will have been populated with
        .ini configuration.
        """
        super(TestDMBringUp, self).__init__(parameters)


    def runTest(self):
        """
        Used by qaautotest module's test runner to run the test case
        and clean up the fixture as necessary.

        With PyUnit, the same method is run although PyUnit will also
        call any defined tearDown method to do test fixture cleanup as well.
        """
        test_passed = True

        if TestCase.pyUnitTCFailure:
            self.log.warning("Skipping Case %s. stop-on-fail/failfast set and a previous test case has failed." %
                             self.__class__.__name__)
            return unittest.skip("stop-on-fail/failfast set and a previous test case has failed.")
        else:
            self.log.info("Running Case %s." % self.__class__.__name__)

        try:
            if not self.test_DMBringUp():
                test_passed = False
        except Exception as inst:
            self.log.error("DM module bringup caused exception:")
            self.log.error(traceback.format_exc())
            test_passed = False

        super(self.__class__, self).reportTestCaseResult(test_passed)

        # If there is any test fixture teardown to be done, do it here.

        if self.parameters["pyUnit"]:
            self.assertTrue(test_passed)
        else:
            return test_passed


    def test_DMBringUp(self):
        """
        Test Case:
        Attempt to start the DM module(s)
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]

        nodes = fdscfg.rt_obj.cfg_nodes
        for n in nodes:
            fds_dir = n.nd_conf_dict['fds_root']
            bin_dir = fdscfg.rt_env.get_bin_dir(debug=False)
            log_dir = fdscfg.rt_env.get_log_dir()

            self.log.info("Start DM on %s." %n.nd_conf_dict['node-name'])

            status = n.nd_agent.exec_wait("%s/DataMgr --fds-root=%s > %s/dm.out" % (bin_dir, fds_dir, log_dir))

            if status != 0:
                self.log.error("DM bringup on %s returned status %d." %(n.nd_conf_dict['node-name'], status))
                return False

            time.sleep(2)

        return True


# This class contains the attributes and methods to test
# waiting for a Data Manager (DM) module to start.
class TestDMWait(TestCase.FDSTestCase):
    def __init__(self, parameters=None):
        """
        When run by a qaautotest module test runner,
        "parameters" will have been populated with
        .ini configuration.
        """
        super(TestDMWait, self).__init__(parameters)


    def runTest(self):
        """
        Used by qaautotest module's test runner to run the test case
        and clean up the fixture as necessary.

        With PyUnit, the same method is run although PyUnit will also
        call any defined tearDown method to do test fixture cleanup as well.
        """
        test_passed = True

        if TestCase.pyUnitTCFailure:
            self.log.warning("Skipping Case %s. stop-on-fail/failfast set and a previous test case has failed." %
                             self.__class__.__name__)
            return unittest.skip("stop-on-fail/failfast set and a previous test case has failed.")
        else:
            self.log.info("Running Case %s." % self.__class__.__name__)

        try:
            if not self.test_DMWait():
                test_passed = False
        except Exception as inst:
            self.log.error("Wait for DM module to start caused exception:")
            self.log.error(traceback.format_exc())
            test_passed = False

        super(self.__class__, self).reportTestCaseResult(test_passed)

        # If there is any test fixture teardown to be done, do it here.

        if self.parameters["pyUnit"]:
            self.assertTrue(test_passed)
        else:
            return test_passed


    def test_DMWait(self):
        """
        Test Case:
        Wait for the DM module(s) to start
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]

        nodes = fdscfg.rt_obj.cfg_nodes
        for n in nodes:
            self.log.info("Wait for DM on %s." %n.nd_conf_dict['node-name'])

            if not modWait("DataMgr", n):
                return False

        return True


# This class contains the attributes and methods to test
# shutting down a Data Manager (DM) module.
class TestDMShutDown(TestCase.FDSTestCase):
    def __init__(self, parameters=None):
        """
        When run by a qaautotest module test runner,
        "parameters" will have been populated with
        .ini configuration.
        """
        super(TestDMShutDown, self).__init__(parameters)


    def runTest(self):
        test_passed = True

        if TestCase.pyUnitTCFailure:
            self.log.warning("Skipping Case %s. stop-on-fail/failfast set and a previous test case has failed." %
                             self.__class__.__name__)
            return unittest.skip("stop-on-fail/failfast set and a previous test case has failed.")
        else:
            self.log.info("Running Case %s." % self.__class__.__name__)

        try:
            if not self.test_DMShutDown():
                test_passed = False
        except Exception as inst:
            self.log.error("DM module shutdown caused exception:")
            self.log.error(traceback.format_exc())
            test_passed = False

        super(self.__class__, self).reportTestCaseResult(test_passed)

        # If there is any test fixture teardown to be done, do it here.

        if self.parameters["pyUnit"]:
            self.assertTrue(test_passed)
        else:
            return test_passed


    def test_DMShutDown(self):
        """
        Test Case:
        Attempt to shutdown the DM module(s)
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]

        nodes = fdscfg.rt_obj.cfg_nodes
        for n in nodes:
            self.log.info("Shutdown DM on %s." %n.nd_conf_dict['node-name'])

            status = n.nd_agent.exec_wait("pkill -9 DataMgr")

            if status != 0:
                self.log.error("DM shutdown on %s returned status %d." %(n.nd_conf_dict['node-name'], status))
                return False

            time.sleep(2)

        return True


# This class contains the attributes and methods to test
# bringing up a Storage Manager (SM) module.
class TestSMBringUp(TestCase.FDSTestCase):
    def __init__(self, parameters=None):
        """
        When run by a qaautotest module test runner,
        "parameters" will have been populated with
        .ini configuration.
        """
        super(TestSMBringUp, self).__init__(parameters)


    def runTest(self):
        """
        Used by qaautotest module's test runner to run the test case
        and clean up the fixture as necessary.

        With PyUnit, the same method is run although PyUnit will also
        call any defined tearDown method to do test fixture cleanup as well.
        """
        test_passed = True

        if TestCase.pyUnitTCFailure:
            self.log.warning("Skipping Case %s. stop-on-fail/failfast set and a previous test case has failed." %
                             self.__class__.__name__)
            return unittest.skip("stop-on-fail/failfast set and a previous test case has failed.")
        else:
            self.log.info("Running Case %s." % self.__class__.__name__)

        try:
            if not self.test_SMBringUp():
                test_passed = False
        except Exception as inst:
            self.log.error("SM module bringup caused exception:")
            self.log.error(traceback.format_exc())
            test_passed = False

        super(self.__class__, self).reportTestCaseResult(test_passed)

        # If there is any test fixture teardown to be done, do it here.

        if self.parameters["pyUnit"]:
            self.assertTrue(test_passed)
        else:
            return test_passed


    def test_SMBringUp(self):
        """
        Test Case:
        Attempt to start the SM module(s)
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]

        nodes = fdscfg.rt_obj.cfg_nodes
        for n in nodes:
            fds_dir = n.nd_conf_dict['fds_root']
            bin_dir = fdscfg.rt_env.get_bin_dir(debug=False)
            log_dir = fdscfg.rt_env.get_log_dir()

            self.log.info("Start SM on %s." %n.nd_conf_dict['node-name'])

            status = n.nd_agent.exec_wait("%s/StorMgr --fds-root=%s > %s/sm.out" % (bin_dir, fds_dir, log_dir))

            if status != 0:
                self.log.error("SM on %s returned status %d." %(n.nd_conf_dict['node-name'], status))
                return False

            time.sleep(2)

        return True


# This class contains the attributes and methods to test
# waiting for a Storage Manager (SM) module to start.
class TestSMWait(TestCase.FDSTestCase):
    def __init__(self, parameters=None):
        """
        When run by a qaautotest module test runner,
        "parameters" will have been populated with
        .ini configuration.
        """
        super(TestSMWait, self).__init__(parameters)


    def runTest(self):
        """
        Used by qaautotest module's test runner to run the test case
        and clean up the fixture as necessary.

        With PyUnit, the same method is run although PyUnit will also
        call any defined tearDown method to do test fixture cleanup as well.
        """
        test_passed = True

        if TestCase.pyUnitTCFailure:
            self.log.warning("Skipping Case %s. stop-on-fail/failfast set and a previous test case has failed." %
                             self.__class__.__name__)
            return unittest.skip("stop-on-fail/failfast set and a previous test case has failed.")
        else:
            self.log.info("Running Case %s." % self.__class__.__name__)

        try:
            if not self.test_SMWait():
                test_passed = False
        except Exception as inst:
            self.log.error("Wait for SM module to start caused exception:")
            self.log.error(traceback.format_exc())
            test_passed = False

        super(self.__class__, self).reportTestCaseResult(test_passed)

        # If there is any test fixture teardown to be done, do it here.

        if self.parameters["pyUnit"]:
            self.assertTrue(test_passed)
        else:
            return test_passed


    def test_SMWait(self):
        """
        Test Case:
        Wait for the SM module(s) to start
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]

        nodes = fdscfg.rt_obj.cfg_nodes
        for n in nodes:
            self.log.info("Wait for SM on %s." %n.nd_conf_dict['node-name'])

            if not modWait("StorMgr", n):
                return False

        return True


# This class contains the attributes and methods to test
# shutting down a Storage Manager (SM) module.
class TestSMShutDown(TestCase.FDSTestCase):
    def __init__(self, parameters=None):
        """
        When run by a qaautotest module test runner,
        "parameters" will have been populated with
        .ini configuration.
        """
        super(TestSMShutDown, self).__init__(parameters)


    def runTest(self):
        test_passed = True

        if TestCase.pyUnitTCFailure:
            self.log.warning("Skipping Case %s. stop-on-fail/failfast set and a previous test case has failed." %
                             self.__class__.__name__)
            return unittest.skip("stop-on-fail/failfast set and a previous test case has failed.")
        else:
            self.log.info("Running Case %s." % self.__class__.__name__)

        try:
            if not self.test_SMShutDown():
                test_passed = False
        except Exception as inst:
            self.log.error("SM module shutdown caused exception:")
            self.log.error(traceback.format_exc())
            test_passed = False

        super(self.__class__, self).reportTestCaseResult(test_passed)

        # If there is any test fixture teardown to be done, do it here.

        if self.parameters["pyUnit"]:
            self.assertTrue(test_passed)
        else:
            return test_passed


    def test_SMShutDown(self):
        """
        Test Case:
        Attempt to shutdown the SM module(s)
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]

        nodes = fdscfg.rt_obj.cfg_nodes
        for n in nodes:
            self.log.info("Shutdown SM on %s." %n.nd_conf_dict['node-name'])

            status = n.nd_agent.exec_wait("pkill -9 StorMgr")

            if status != 0:
                self.log.error("SM shutdown on %s returned status %d." %(n.nd_conf_dict['node-name'], status))
                return False

            time.sleep(2)

        return True


# This class contains the attributes and methods to test
# bringing up a Platform Manager (SM) module.
class TestPMBringUp(TestCase.FDSTestCase):
    def __init__(self, parameters=None):
        """
        When run by a qaautotest module test runner,
        "parameters" will have been populated with
        .ini configuration.
        """
        super(TestPMBringUp, self).__init__(parameters)


    def runTest(self):
        test_passed = True

        if TestCase.pyUnitTCFailure:
            self.log.warning("Skipping Case %s. stop-on-fail/failfast set and a previous test case has failed." %
                             self.__class__.__name__)
            return unittest.skip("stop-on-fail/failfast set and a previous test case has failed.")
        else:
            self.log.info("Running Case %s." % self.__class__.__name__)

        try:
            if not self.test_PMBringUp():
                test_passed = False
        except Exception as inst:
            self.log.error("PM module bringup caused exception:")
            self.log.error(traceback.format_exc())
            test_passed = False

        super(self.__class__, self).reportTestCaseResult(test_passed)

        # If there is any test fixture teardown to be done, do it here.

        if self.parameters["pyUnit"]:
            self.assertTrue(test_passed)
        else:
            return test_passed


    def test_PMBringUp(self):
        """
        Test Case:
        Attempt to start the PM module(s)
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]

        nodes = fdscfg.rt_obj.cfg_nodes
        for n in nodes:
            bin_dir = fdscfg.rt_env.get_bin_dir(debug=False)
            log_dir = fdscfg.rt_env.get_log_dir()
            om_node = fdscfg.rt_om_node
            om_ip = om_node.nd_conf_dict['ip']

            self.log.info("Start PM on %s." %n.nd_conf_dict['node-name'])

            status = n.nd_start_platform(om_ip, test_harness=True, _bin_dir=bin_dir, _log_dir=log_dir)

            if status != 0:
                self.log.error("PM on %s returned status %d." %(n.nd_conf_dict['node-name'], status))
                return False

        return True


# This class contains the attributes and methods to test
# waiting for a Platform Manager (PM) module to start.
class TestPMWait(TestCase.FDSTestCase):
    def __init__(self, parameters=None):
        """
        When run by a qaautotest module test runner,
        "parameters" will have been populated with
        .ini configuration.
        """
        super(TestPMWait, self).__init__(parameters)


    def runTest(self):
        """
        Used by qaautotest module's test runner to run the test case
        and clean up the fixture as necessary.

        With PyUnit, the same method is run although PyUnit will also
        call any defined tearDown method to do test fixture cleanup as well.
        """
        test_passed = True

        if TestCase.pyUnitTCFailure:
            self.log.warning("Skipping Case %s. stop-on-fail/failfast set and a previous test case has failed." %
                             self.__class__.__name__)
            return unittest.skip("stop-on-fail/failfast set and a previous test case has failed.")
        else:
            self.log.info("Running Case %s." % self.__class__.__name__)

        try:
            if not self.test_PMWait():
                test_passed = False
        except Exception as inst:
            self.log.error("Wait for PM module to start caused exception:")
            self.log.error(traceback.format_exc())
            test_passed = False

        super(self.__class__, self).reportTestCaseResult(test_passed)

        # If there is any test fixture teardown to be done, do it here.

        if self.parameters["pyUnit"]:
            self.assertTrue(test_passed)
        else:
            return test_passed


    def test_PMWait(self):
        """
        Test Case:
        Wait for the PM module(s) to start
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]

        nodes = fdscfg.rt_obj.cfg_nodes
        for n in nodes:
            self.log.info("Wait for PM on %s." %n.nd_conf_dict['node-name'])

            if not modWait("platformd", n):
                return False

        return True


# This class contains the attributes and methods to test
# shutting down a Platform Manager (PM) module.
class TestPMShutDown(TestCase.FDSTestCase):
    def __init__(self, parameters=None):
        """
        When run by a qaautotest module test runner,
        "parameters" will have been populated with
        .ini configuration.
        """
        super(TestPMShutDown, self).__init__(parameters)


    def runTest(self):
        test_passed = True

        if TestCase.pyUnitTCFailure:
            self.log.warning("Skipping Case %s. stop-on-fail/failfast set and a previous test case has failed." %
                             self.__class__.__name__)
            return unittest.skip("stop-on-fail/failfast set and a previous test case has failed.")
        else:
            self.log.info("Running Case %s." % self.__class__.__name__)

        try:
            if not self.test_PMShutDown():
                test_passed = False
        except Exception as inst:
            self.log.error("PM module shutdown caused exception:")
            self.log.error(traceback.format_exc())
            test_passed = False

        super(self.__class__, self).reportTestCaseResult(test_passed)

        # If there is any test fixture teardown to be done, do it here.

        if self.parameters["pyUnit"]:
            self.assertTrue(test_passed)
        else:
            return test_passed


    def test_PMShutDown(self):
        """
        Test Case:
        Attempt to shutdown the PM module(s)
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]

        nodes = fdscfg.rt_obj.cfg_nodes
        for n in nodes:
            self.log.info("Shutdown PM on %s." %n.nd_conf_dict['node-name'])

            status = n.nd_agent.exec_wait("pkill -9 platformd")

            if status != 0:
                self.log.error("PM shutdown on %s returned status %d." %(n.nd_conf_dict['node-name'], status))
                return False

            time.sleep(2)

        return True


# This class contains the attributes and methods to test
# bringing up an Orchestration Manager (OM) module.
class TestOMBringUp(TestCase.FDSTestCase):
    def __init__(self, parameters=None):
        """
        When run by a qaautotest module test runner,
        "parameters" will have been populated with
        .ini configuration.
        """
        super(TestOMBringUp, self).__init__(parameters)


    def runTest(self):
        test_passed = True

        if TestCase.pyUnitTCFailure:
            self.log.warning("Skipping Case %s. stop-on-fail/failfast set and a previous test case has failed." %
                             self.__class__.__name__)
            return unittest.skip("stop-on-fail/failfast set and a previous test case has failed.")
        else:
            self.log.info("Running Case %s." % self.__class__.__name__)

        try:
            if not self.test_OMBringUp():
                test_passed = False
        except Exception as inst:
            self.log.error("OM module bringup caused exception:")
            self.log.error(traceback.format_exc())
            test_passed = False

        super(self.__class__, self).reportTestCaseResult(test_passed)

        # If there is any test fixture teardown to be done, do it here.

        if self.parameters["pyUnit"]:
            self.assertTrue(test_passed)
        else:
            return test_passed


    def test_OMBringUp(self):
        """
        Test Case:
        Attempt to start the OM module(s)
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]

        bin_dir = fdscfg.rt_env.get_bin_dir(debug=False)
        log_dir = fdscfg.rt_env.get_log_dir()
        om_node = fdscfg.rt_om_node

        self.log.info("Start OM on %s." % om_node.nd_conf_dict['node-name'])

        status = om_node.nd_start_om(test_harness=True, _bin_dir=bin_dir, _log_dir=log_dir)

        if status != 0:
            self.log.error("OM on %s returned status %d." % (om_node.nd_conf_dict['node-name'], status))
            return False

        return True


# This class contains the attributes and methods to wait
# for the Orchestration Manager (OM) module to become active.
class TestOMWait(TestCase.FDSTestCase):
    def __init__(self, parameters=None):
        """
        When run by a qaautotest module test runner,
        "parameters" will have been populated with
        .ini configuration.
        """
        super(TestOMWait, self).__init__(parameters)


    def runTest(self):
        test_passed = True

        if TestCase.pyUnitTCFailure:
            self.log.warning("Skipping Case %s. stop-on-fail/failfast set and a previous test case has failed." %
                             self.__class__.__name__)
            return unittest.skip("stop-on-fail/failfast set and a previous test case has failed.")
        else:
            self.log.info("Running Case %s." % self.__class__.__name__)

        try:
            if not self.test_OMWait():
                test_passed = False
        except Exception as inst:
            self.log.error("OM module wait caused exception:")
            self.log.error(traceback.format_exc())
            test_passed = False

        super(self.__class__, self).reportTestCaseResult(test_passed)

        # If there is any test fixture teardown to be done, do it here.

        if self.parameters["pyUnit"]:
            self.assertTrue(test_passed)
        else:
            return test_passed


    def test_OMWait(self):
        """
        Test Case:
        Wait for the named module to become active.
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]

        self.log.info("Wait for OM on %s." % fdscfg.rt_om_node.nd_conf_dict['node-name'])

        return modWait("orchMgr", fdscfg.rt_om_node)


# This class contains the attributes and methods to test
# shutting down an Orchestration Manager (OM) module.
class TestOMShutDown(TestCase.FDSTestCase):
    def __init__(self, parameters=None):
        """
        When run by a qaautotest module test runner,
        "parameters" will have been populated with
        .ini configuration.
        """
        super(TestOMShutDown, self).__init__(parameters)


    def runTest(self):
        test_passed = True

        if TestCase.pyUnitTCFailure:
            self.log.warning("Skipping Case %s. stop-on-fail/failfast set and a previous test case has failed." %
                             self.__class__.__name__)
            return unittest.skip("stop-on-fail/failfast set and a previous test case has failed.")
        else:
            self.log.info("Running Case %s." % self.__class__.__name__)

        try:
            if not self.test_OMShutDown():
                test_passed = False
        except Exception as inst:
            self.log.error("OM module shutdown caused exception:")
            self.log.error(traceback.format_exc())
            test_passed = False

        super(self.__class__, self).reportTestCaseResult(test_passed)

        # If there is any test fixture teardown to be done, do it here.

        if self.parameters["pyUnit"]:
            self.assertTrue(test_passed)
        else:
            return test_passed


    def test_OMShutDown(self):
        """
        Test Case:
        Attempt to shutdown the OM module(s)
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]

        om_node = fdscfg.rt_om_node

        self.log.info("Shutdown OM on %s." % om_node.nd_conf_dict['node-name'])

        status = om_node.nd_agent.exec_wait("pkill -9 orchMgr")

        if status != 0:
            self.log.error("OM (orchMgr) shutdown on %s returned status %d." %(om_node.nd_conf_dict['node-name'], status))
            return False

        status = om_node.nd_agent.exec_wait('pkill -9 -f com.formationds.om.Main')

        if status != 0:
            self.log.error("OM (com.formationds.om.Main) shutdown on %s returned status %d." %(om_node.nd_conf_dict['node-name'], status))
            return False

        time.sleep(2)

        return True


# This class contains the attributes and methods to test
# waiting for an Access Manager (AM) module to start.
class TestAMWait(TestCase.FDSTestCase):
    def __init__(self, parameters=None):
        """
        When run by a qaautotest module test runner,
        "parameters" will have been populated with
        .ini configuration.
        """
        super(TestAMWait, self).__init__(parameters)


    def runTest(self):
        """
        Used by qaautotest module's test runner to run the test case
        and clean up the fixture as necessary.

        With PyUnit, the same method is run although PyUnit will also
        call any defined tearDown method to do test fixture cleanup as well.
        """
        test_passed = True

        if TestCase.pyUnitTCFailure:
            self.log.warning("Skipping Case %s. stop-on-fail/failfast set and a previous test case has failed." %
                             self.__class__.__name__)
            return unittest.skip("stop-on-fail/failfast set and a previous test case has failed.")
        else:
            self.log.info("Running Case %s." % self.__class__.__name__)

        try:
            if not self.test_AMWait():
                test_passed = False
        except Exception as inst:
            self.log.error("Wait for AM module to start caused exception:")
            self.log.error(traceback.format_exc())
            test_passed = False

        super(self.__class__, self).reportTestCaseResult(test_passed)

        # If there is any test fixture teardown to be done, do it here.

        if self.parameters["pyUnit"]:
            self.assertTrue(test_passed)
        else:
            return test_passed


    def test_AMWait(self):
        """
        Test Case:
        Wait for the AM module(s) to start
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]

        nodes = fdscfg.rt_get_obj('cfg_am')
        for n in nodes:
            self.log.info("Wait for AM on %s." % n.nd_conf_dict['fds_node'])

            if not modWait("AMAgent", n.nd_am_node):
                return False

        return True


# This class contains the attributes and methods to test
# shutting down an Access Manager (AM) module.
class TestAMShutDown(TestCase.FDSTestCase):
    def __init__(self, parameters=None):
        """
        When run by a qaautotest module test runner,
        "parameters" will have been populated with
        .ini configuration.
        """
        super(TestAMShutDown, self).__init__(parameters)


    def runTest(self):
        test_passed = True

        if TestCase.pyUnitTCFailure:
            self.log.warning("Skipping Case %s. stop-on-fail/failfast set and a previous test case has failed." %
                             self.__class__.__name__)
            return unittest.skip("stop-on-fail/failfast set and a previous test case has failed.")
        else:
            self.log.info("Running Case %s." % self.__class__.__name__)

        try:
            if not self.test_AMShutDown():
                test_passed = False
        except Exception as inst:
            self.log.error("AM module shutdown caused exception:")
            self.log.error(traceback.format_exc())
            test_passed = False

        super(self.__class__, self).reportTestCaseResult(test_passed)

        # If there is any test fixture teardown to be done, do it here.

        if self.parameters["pyUnit"]:
            self.assertTrue(test_passed)
        else:
            return test_passed


    def test_AMShutDown(self):
        """
        Test Case:
        Attempt to shutdown the AM module(s)
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]

        nodes = fdscfg.rt_get_obj('cfg_am')
        for n in nodes:
            self.log.info("Shutdown AM on %s." % n.nd_conf_dict['node-name'])

            status = n.nd_agent.exec_wait("pkill -9 -f com.formationds.am.Main;")

            if status != 0:
                self.log.error("AM (com.formationds.am.Main) shutdown on %s returned status %d." %(n.nd_conf_dict['node-name'], status))
                return False

            status = n.nd_agent.exec_wait('pkill -9 bare_am')

            if status != 0:
                self.log.error("AM (bare_am) shutdown on %s returned status %d." %(n.nd_conf_dict['node-name'], status))
                return False

            status = n.nd_agent.exec_wait('pkill -9 AMAgent')

            if status != 0:
                self.log.error("AM (AMAgent) shutdown on %s returned status %d." %(n.nd_conf_dict['node-name'], status))
                return False

            status = n.nd_agent.exec_wait("pkill -9 -f com.formationds.om.Main;")

            if status != 0:
                self.log.error("AM (com.formationds.om.Main) shutdown on %s returned status %d." %(n.nd_conf_dict['node-name'], status))
                return False

            time.sleep(2)

        return True


if __name__ == '__main__':
    TestCase.FDSTestCase.fdsGetCmdLineConfigs(sys.argv)
    unittest.main(testRunner=xmlrunner.XMLTestRunner(output='test-reports'))