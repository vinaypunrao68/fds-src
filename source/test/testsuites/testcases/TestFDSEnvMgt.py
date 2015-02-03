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
import os
import time


# This class contains attributes and methods to test
# creating an FDS installation from a development environment.
class TestFDSCreateInstDir(TestCase.FDSTestCase):
    def __init__(self, parameters=None, node=None, nodeID=-1):
        """
        When run by a qaautotest module test runner,
        "parameters" will have been populated with
        .ini configuration.
        """
        super(self.__class__, self).__init__(parameters)

        self.passedNode = node
        self.nodeID = nodeID


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
            if not self.test_FDSCreateInstDir():
                test_passed = False
        except Exception as inst:
            self.log.error("FDS installation directory creation caused exception:")
            self.log.error(traceback.format_exc())
            test_passed = False

        super(self.__class__, self).reportTestCaseResult(test_passed)

        # If there is any test fixture teardown to be done, do it here.

        if self.parameters["pyUnit"]:
            self.assertTrue(test_passed)
        else:
            return test_passed


    def test_FDSCreateInstDir(self):
        """
        Test Case:
        Attempt to create the FDS installation directory structure
        for an installation based upon a development environment.
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]

        nodes = fdscfg.rt_obj.cfg_nodes
        amnodes = fdscfg.rt_get_obj('cfg_am')
        instanceId = 0
        amn = amnodes[instanceId]
        for n in nodes:
            # If we were passed a node, use it and get out. Otherwise,
            # we spin through all defined nodes setting them up.
            if self.passedNode is not None:
                n = self.passedNode
                assert(self.nodeID >= 0)

            if not n.nd_agent.env_install:
                fds_dir = n.nd_conf_dict['fds_root']

                # Check to see if the FDS root directory is already there.
                status = n.nd_agent.exec_wait('ls ' + fds_dir)
                if status != 0:
                    # Not there, try to create it.
                    self.log.info("FDS installation directory, %s, nonexistent on node %s. Attempting to create." %
                                  (fds_dir, n.nd_conf_dict['node-name']))
                    status = n.nd_agent.exec_wait('mkdir -p ' + fds_dir)
                    if status != 0:
                        self.log.error("FDS installation directory creation on node %s returned status %d." %
                                       (n.nd_conf_dict['node-name'], status))
                        return False
                else:
                    self.log.warn("FDS installation directory, %s, exists on node %s." %
                                  (fds_dir, n.nd_conf_dict['node-name']))

                # Populate application configuration directory if necessary.
                # It should be necessary since we did not do a package install.
                dest_config_dir = fds_dir + "/etc"
                # Check to see if the etc directory is already there.
                status = n.nd_agent.exec_wait('ls ' + dest_config_dir)
                if status != 0:
                    # Not there, try to create it.
                    self.log.info("FDS configuration directory, %s, nonexistent on node %s. Attempting to create." %
                                  (dest_config_dir, n.nd_conf_dict['node-name']))
                    status = n.nd_agent.exec_wait('mkdir -p ' + dest_config_dir)
                    if status != 0:
                        self.log.error("FDS configuration directory creation on node %s returned status %d." %
                                       (n.nd_conf_dict['node-name'], status))
                        return False
                else:
                    self.log.warn("FDS configuration directory, %s, exists on node %s." %
                                  (dest_config_dir, n.nd_conf_dict['node-name']))

                # Load the product configuration directory from the development environment.
                src_config_dir = n.nd_agent.get_config_dir()
                status = n.nd_agent.exec_wait('cp -rf ' + src_config_dir + ' ' + fds_dir)
                if status != 0:
                    self.log.error("FDS configuration directory population on node %s returned status %d." %
                                   (n.nd_conf_dict['node-name'], status))
                    return False

                # Make sure the platform config file specifies the correct PM port and
                # AM instance ID and port configurations.
                if 'fds_port' in n.nd_conf_dict:
                    port = n.nd_conf_dict['fds_port']
                else:
                    self.log.error("FDS platform configuration file is missing 'platform_port = ' config")
                    return False

                status = n.nd_agent.exec_wait('rm %s/platform.conf ' % dest_config_dir)

                # Fix up the platform config file for this node.
                if self.passedNode is not None:
                    instanceId = self.nodeID

                # Obtain these defaults from platform.conf.
                s3_http_port = 8000 + instanceId
                s3_https_port = 8443 + instanceId
                swift_port = 9999 + instanceId
                nbd_server_port = 10809 + instanceId
                status = n.nd_agent.exec_wait('sed -e "s/ platform_port = 7000/ platform_port = %s/g" '
                                              '-e "s/ instanceId = 0/ instanceId = %s/g" '
                                              '-e "s/ s3_http_port=8000/ s3_http_port=%s/g" '
                                              '-e "s/ s3_https_port=8443/ s3_https_port=%s/g" '
                                              '-e "s/ swift_port=9999/ swift_port=%s/g" '
                                              '-e "s/ nbd_server_port = 10809/ nbd_server_port = %s/g" '
                                              '-e "1,$w %s/platform.conf" '
                                              '%s/platform.conf ' %
                                              (port, instanceId, s3_http_port, s3_https_port,
                                               swift_port, nbd_server_port,
                                               dest_config_dir, src_config_dir))

                if status != 0:
                    self.log.error("FDS platform configuration file modification failed.")
                    return False
                elif self.passedNode is not None:
                    # If we were passed a specific node, just take care
                    # of that one and exit.
                    return True

                # Bump instanceID if necessary.
                if amn.nd_conf_dict['fds_node'] == n.nd_conf_dict['node-name']:
                    instanceId = instanceId + 1
                    if len(amnodes) < instanceId:
                        amn = amnodes[instanceId]

            else:
                self.log.error("Test method %s is meant to be called when testing "
                               "from a development environment. Instead, try class TestFDSPkgInst." %
                               sys._getframe().f_code.co_name)
                return False

        return True


# This class contains attributes and methods to test
# creating an FDS installation from an FDS installation package.
class TestFDSPkgInst(TestCase.FDSTestCase):
    def __init__(self, parameters = None):
        """
        When run by a qaautotest module test runner,
        "parameters" will have been populated with
        .ini configuration.
        """
        super(self.__class__, self).__init__(parameters)


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
            if not self.test_FDSPkgInst():
                test_passed = False
        except Exception as inst:
            self.log.error("FDS package installation caused exception:")
            self.log.error(traceback.format_exc())
            test_passed = False

        super(self.__class__, self).reportTestCaseResult(test_passed)

        # If there is any test fixture teardown to be done, do it here.

        if self.parameters["pyUnit"]:
            self.assertTrue(test_passed)
        else:
            return test_passed


    def test_FDSPkgInst(self):
        """
        Test Case:
        Attempt to install the FDS package onto the specified nodes.
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]

        nodes = fdscfg.rt_obj.cfg_nodes
        for n in nodes:
            status = n.nd_install_rmt_pkg()

            if status != 0:
                self.log.error("FDS package installation for node %s returned status %d." %
                               (n.nd_conf_dict['node-name'], status))
                return False

        return True


# This class contains attributes and methods to test
# deleting an FDS installation or root directory.
class TestFDSDeleteInstDir(TestCase.FDSTestCase):
    def __init__(self, parameters = None, node=None):
        """
        When run by a qaautotest module test runner,
        "parameters" will have been populated with
        .ini configuration.
        """
        super(self.__class__, self).__init__(parameters)

        self.passedNode = node


    @unittest.expectedFailure
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
            if not self.test_FDSDeleteInstDir():
                test_passed = False
        except Exception as inst:
            self.log.error("FDS installation directory deletion caused exception:")
            self.log.error(traceback.format_exc())
            test_passed = False

        super(self.__class__, self).reportTestCaseResult(test_passed)

        # If there is any test fixture teardown to be done, do it here.

        if self.parameters["pyUnit"]:
            self.assertTrue(test_passed)
        else:
            return test_passed


    def test_FDSDeleteInstDir(self):
        """
        Test Case:
        Attempt to delete the FDS installation directory structure.
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]
        bin_dir = fdscfg.rt_env.get_bin_dir(debug=False)

        nodes = fdscfg.rt_obj.cfg_nodes
        for n in nodes:
            # If we were passed a specific node, use it and get it.
            if self.passedNode is not None:
                n = self.passedNode

            fds_dir = n.nd_conf_dict['fds_root']

            # Check to see if the FDS root directory is already there.
            status = n.nd_agent.exec_wait('ls ' + fds_dir)
            if status == 0:
                # Try to delete it.
                self.log.info("FDS installation directory, %s, exists on node %s. Attempting to delete." %
                              (fds_dir, n.nd_conf_dict['node-name']))

                status = n.nd_agent.exec_wait('rm -rf %s ' % fds_dir)

                if status != 0:
                    self.log.error("FDS installation directory deletion on node %s returned status %d." %
                                   (n.nd_conf_dict['node-name'], status))
                    return False
            else:
                self.log.warn("FDS installation directory, %s, nonexistent on node %s." %
                              (fds_dir, n.nd_conf_dict['node-name']))

            if self.passedNode is not None:
                # We're done with the specified node. Get out.
                break

        return True


# This class contains attributes and methods to test clean shared memory.
# A workaround that is presently required if you want to restart a cluster
# that was previously started.
class TestFDSSharedMemoryClean(TestCase.FDSTestCase):
    def __init__(self, parameters = None, node=None):
        """
        When run by a qaautotest module test runner,
        "parameters" will have been populated with
        .ini configuration.
        """
        super(self.__class__, self).__init__(parameters)

        self.passedNode = node


    @unittest.expectedFailure
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
            if not self.test_FDSSharedMemoryClean():
                test_passed = False
        except Exception as inst:
            self.log.error("Shared Memory cleanup caused exception:")
            self.log.error(traceback.format_exc())
            test_passed = False

    def test_FDSSharedMemoryClean(self):
        """
        Test Case:
        Attempt to selectively delete from the FDS installation directory.
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]
        bin_dir = fdscfg.rt_env.get_bin_dir(debug=False)

        nodes = fdscfg.rt_obj.cfg_nodes
        for n in nodes:
            # If we were passed a specific node, use it and get it.
            if self.passedNode is not None:
                n = self.passedNode

            # Check to see if shared memory segments exist
            status = n.nd_agent.exec_wait('ls /dev/shm/0x*')
            if status == 0:
                # Try to delete it.
                self.log.info("Shared memory segments exist, attempting to remove them on node %s" %
                              (n.nd_conf_dict['node-name']))

                status = n.nd_agent.exec_wait('rm /dev/shm/0x*')
                if status != 0:
                    self.log.error("Shared memory cleanup on node %s returned status %d." %
                                   (n.nd_conf_dict['node-name'], status))
                    return False
            else:
                self.log.warn("Shared memory segments nonexistent on node %s." %
                              (n.nd_conf_dict['node-name']))

            if self.passedNode is not None:
                # We're done with the specified node. Get out.
                break

        return True

# This class contains attributes and methods to test
# clean selective parts of an FDS installation directory.
class TestFDSSelectiveInstDirClean(TestCase.FDSTestCase):
    def __init__(self, parameters = None, node=None):
        """
        When run by a qaautotest module test runner,
        "parameters" will have been populated with
        .ini configuration.
        """
        super(self.__class__, self).__init__(parameters)

        self.passedNode = node


    @unittest.expectedFailure
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
            if not self.test_FDSSelectiveInstDirClean():
                test_passed = False
        except Exception as inst:
            self.log.error("Selective FDS installation directory clean caused exception:")
            self.log.error(traceback.format_exc())
            test_passed = False

        super(self.__class__, self).reportTestCaseResult(test_passed)

        # If there is any test fixture teardown to be done, do it here.

        if self.parameters["pyUnit"]:
            self.assertTrue(test_passed)
        else:
            return test_passed


    def test_FDSSelectiveInstDirClean(self):
        """
        Test Case:
        Attempt to selectively delete from the FDS installation directory.
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]
        bin_dir = fdscfg.rt_env.get_bin_dir(debug=False)

        nodes = fdscfg.rt_obj.cfg_nodes
        for n in nodes:
            # If we were passed a specific node, use it and get it.
            if self.passedNode is not None:
                n = self.passedNode

            fds_dir = n.nd_conf_dict['fds_root']

            # Check to see if the FDS root directory is already there.
            status = n.nd_agent.exec_wait('ls ' + fds_dir)
            if status == 0:
                # Try to delete it.
                self.log.info("FDS installation directory, %s, exists on node %s. Attempting to selectively clean." %
                              (fds_dir, n.nd_conf_dict['node-name']))
                print fds_dir
                status = n.nd_cleanup_node(test_harness=True, _bin_dir=bin_dir)
                if status != 0:
                    self.log.error("FDS installation directory selective clean on node %s returned status %d." %
                                   (n.nd_conf_dict['node-name'], status))
                    return False
            else:
                self.log.warn("FDS installation directory, %s, nonexistent on node %s." %
                              (fds_dir, n.nd_conf_dict['node-name']))

            if self.passedNode is not None:
                # We're done with the specified node. Get out.
                break

        return True


# This class contains the attributes and methods to test
# restarting Redis to a "clean" state.
class TestRestartRedisClean(TestCase.FDSTestCase):
    def __init__(self, parameters=None, node=None):
        """
        When run by a qaautotest module test runner,
        "parameters" will have been populated with
        .ini configuration.
        """
        super(TestRestartRedisClean, self).__init__(parameters)

        self.passedNode = node


    def runTest(self):
        test_passed = True

        if TestCase.pyUnitTCFailure:
            self.log.warning("Skipping Case %s. stop-on-fail/failfast set and a previous test case has failed." %
                             self.__class__.__name__)
            return unittest.skip("stop-on-fail/failfast set and a previous test case has failed.")
        else:
            self.log.info("Running Case %s." % self.__class__.__name__)

        try:
            if not self.test_RestartRedisClean():
                test_passed = False
        except Exception as inst:
            self.log.error("Restart Redis clean caused exception:")
            self.log.error(traceback.format_exc())
            test_passed = False

        super(self.__class__, self).reportTestCaseResult(test_passed)

        # If there is any test fixture teardown to be done, do it here.

        if self.parameters["pyUnit"]:
            self.assertTrue(test_passed)
        else:
            return test_passed


    def test_RestartRedisClean(self):
        """
        Test Case:
        Attempt to restart Redis to a "clean" state.
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]

        # TODO(Greg): Probably in the config file we need to indicate by node whether it is
        # based on an install or a development environment. For now, we check for an install
        # directory structure for Redis support. If not there, we assume a development environment.
        if fdscfg.rt_env.env_install:
            sbin_dir = fdscfg.rt_env.get_sbin_dir()
        else:
            sbin_dir = os.path.join(fdscfg.rt_env.get_fds_source(), 'tools')

        # If we were passed a node, use it and get out. Otherwise,
        # we use the one captured as the OM node during config parsing.
        if self.passedNode is not None:
            n = self.passedNode
        else:
            n = fdscfg.rt_om_node

        self.log.info("Restart Redis clean on %s." %n.nd_conf_dict['node-name'])

        status = n.nd_agent.exec_wait("%s/redis.sh restart" % sbin_dir)
        time.sleep(2)

        if status != 0:
            self.log.error("Restart Redis before clean on %s returned status %d." %(n.nd_conf_dict['node-name'], status))
            return False

        status = n.nd_agent.exec_wait("%s/redis.sh clean" % sbin_dir)
        time.sleep(2)

        if status != 0:
            self.log.error("Clean Redis on %s returned status %d." %(n.nd_conf_dict['node-name'], status))
            return False

        status = n.nd_agent.exec_wait("%s/redis.sh restart" % sbin_dir)
        time.sleep(2)

        if status != 0:
            self.log.error("Restart Redis after clean on %s returned status %d." %(n.nd_conf_dict['node-name'], status))
            return False

        return True


# This class contains the attributes and methods to test
# booting up Redis.
class TestBootRedis(TestCase.FDSTestCase):
    def __init__(self, parameters=None, node=None):
        """
        When run by a qaautotest module test runner,
        "parameters" will have been populated with
        .ini configuration.
        """
        super(self.__class__, self).__init__(parameters)

        self.passedNode = node


    def runTest(self):
        test_passed = True

        if TestCase.pyUnitTCFailure:
            self.log.warning("Skipping Case %s. stop-on-fail/failfast set and a previous test case has failed." %
                             self.__class__.__name__)
            return unittest.skip("stop-on-fail/failfast set and a previous test case has failed.")
        else:
            self.log.info("Running Case %s." % self.__class__.__name__)

        try:
            if not self.test_BootRedis():
                test_passed = False
        except Exception as inst:
            self.log.error("Boot Redis caused exception:")
            self.log.error(traceback.format_exc())
            test_passed = False

        super(self.__class__, self).reportTestCaseResult(test_passed)

        # If there is any test fixture teardown to be done, do it here.

        if self.parameters["pyUnit"]:
            self.assertTrue(test_passed)
        else:
            return test_passed


    def test_BootRedis(self):
        """
        Test Case:
        Attempt to boot Redis.
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]

        # TODO(Greg): Probably in the config file we need to indicate by node whether it is
        # based on an install or a development environment. For now, we check for an install
        # directory structure for Redis support. If not there, we assume a development environment.
        if fdscfg.rt_env.env_install:
            sbin_dir = fdscfg.rt_env.get_sbin_dir()
        else:
            sbin_dir = os.path.join(fdscfg.rt_env.get_fds_source(), 'tools')

        # If we were passed a node, use it and get out. Otherwise,
        # we use the one captured as the OM node during config parsing.
        if self.passedNode is not None:
            n = self.passedNode
        else:
            n = fdscfg.rt_om_node

        self.log.info("Boot Redis on node %s." %n.nd_conf_dict['node-name'])

        status = n.nd_agent.exec_wait("%s/redis.sh start" % sbin_dir)
        time.sleep(2)

        if status != 0:
            self.log.error("Boot Redis on node %s returned status %d." % (n.nd_conf_dict['node-name'], status))
            return False

        return True


# This class contains the attributes and methods to test
# shutting down Redis.
class TestShutdownRedis(TestCase.FDSTestCase):
    def __init__(self, parameters=None, node=None):
        """
        When run by a qaautotest module test runner,
        "parameters" will have been populated with
        .ini configuration.
        """
        super(self.__class__, self).__init__(parameters)

        self.passedNode = node


    def runTest(self):
        test_passed = True

        # We'll continue to shutdown Redis even in the event of failure upstream.
        if TestCase.pyUnitTCFailure and not True:
            self.log.warning("Skipping Case %s. stop-on-fail/failfast set and a previous test case has failed." %
                             self.__class__.__name__)
            return unittest.skip("stop-on-fail/failfast set and a previous test case has failed.")
        else:
            self.log.info("Running Case %s." % self.__class__.__name__)

        try:
            if not self.test_ShutdownRedis():
                test_passed = False
        except Exception as inst:
            self.log.error("Shutdown Redis caused exception:")
            self.log.error(traceback.format_exc())
            test_passed = False

        super(self.__class__, self).reportTestCaseResult(test_passed)

        # If there is any test fixture teardown to be done, do it here.

        if self.parameters["pyUnit"]:
            self.assertTrue(test_passed)
        else:
            return test_passed


    def test_ShutdownRedis(self):
        """
        Test Case:
        Attempt to shutdown Redis.
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]

        # TODO(Greg): Probably in the config file we need to indicate by node whether it is
        # based on an install or a development environment. For now, we check for an install
        # directory structure for Redis support. If not there, we assume a development environment.
        if fdscfg.rt_env.env_install:
            sbin_dir = fdscfg.rt_env.get_sbin_dir()
        else:
            sbin_dir = os.path.join(fdscfg.rt_env.get_fds_source(), 'tools')

        # If we were passed a node, use it and get out. Otherwise,
        # we use the one captured as the OM node during config parsing.
        if self.passedNode is not None:
            n = self.passedNode
        else:
            n = fdscfg.rt_om_node

        self.log.info("Shutdown Redis on node %s." %n.nd_conf_dict['node-name'])

        status = n.nd_agent.exec_wait("%s/redis.sh stop" % sbin_dir)
        time.sleep(2)

        if status != 0:
            self.log.error("Boot Redis on node %s returned status %d." % (n.nd_conf_dict['node-name'], status))
            return False

        return True


# This class contains the attributes and methods to test
# verifying that Redis is up.
class TestVerifyRedisUp(TestCase.FDSTestCase):
    def __init__(self, parameters=None, node=None):
        """
        When run by a qaautotest module test runner,
        "parameters" will have been populated with
        .ini configuration.
        """
        super(self.__class__, self).__init__(parameters)

        self.passedNode = node


    def runTest(self):
        test_passed = True

        if TestCase.pyUnitTCFailure:
            self.log.warning("Skipping Case %s. stop-on-fail/failfast set and a previous test case has failed." %
                             self.__class__.__name__)
            return unittest.skip("stop-on-fail/failfast set and a previous test case has failed.")
        else:
            self.log.info("Running Case %s." % self.__class__.__name__)

        try:
            if not self.test_VerifyRedisUp():
                test_passed = False
        except Exception as inst:
            self.log.error("Verify Redis is up caused exception:")
            self.log.error(traceback.format_exc())
            test_passed = False

        super(self.__class__, self).reportTestCaseResult(test_passed)

        # If there is any test fixture teardown to be done, do it here.

        if self.parameters["pyUnit"]:
            self.assertTrue(test_passed)
        else:
            return test_passed


    def test_VerifyRedisUp(self):
        """
        Test Case:
        Attempt to verify Redis is up.
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]

        # TODO(Greg): Probably in the config file we need to indicate by node whether it is
        # based on an install or a development environment. For now, we check for an install
        # directory structure for Redis support. If not there, we assume a development environment.
        if fdscfg.rt_env.env_install:
            sbin_dir = fdscfg.rt_env.get_sbin_dir()
        else:
            sbin_dir = os.path.join(fdscfg.rt_env.get_fds_source(), 'tools')

        # If we were passed a node, use it and get out. Otherwise,
        # we use the one captured as the OM node during config parsing.
        if self.passedNode is not None:
            n = self.passedNode
        else:
            n = fdscfg.rt_om_node

        self.log.info("Verify Redis is up node %s." % n.nd_conf_dict['node-name'])

        # Parameter return_stdin is set to return stdout. ... Don't ask me!
        status, stdout = n.nd_agent.exec_wait("%s/redis.sh status" % sbin_dir, return_stdin=True)

        if status != 0:
            self.log.error("Verify Redis is up on node %s returned status %d." % (n.nd_conf_dict['node-name'], status))
            return False

        self.log.error(stdout)

        if stdout.count("NOT") > 0:
            return False

        return True


# This class contains the attributes and methods to test
# verifying that Redis is down.
class TestVerifyRedisDown(TestCase.FDSTestCase):
    def __init__(self, parameters=None, node=None):
        """
        When run by a qaautotest module test runner,
        "parameters" will have been populated with
        .ini configuration.
        """
        super(self.__class__, self).__init__(parameters)

        self.passedNode = node


    def runTest(self):
        test_passed = True

        if TestCase.pyUnitTCFailure:
            self.log.warning("Skipping Case %s. stop-on-fail/failfast set and a previous test case has failed." %
                             self.__class__.__name__)
            return unittest.skip("stop-on-fail/failfast set and a previous test case has failed.")
        else:
            self.log.info("Running Case %s." % self.__class__.__name__)

        try:
            if not self.test_VerifyRedisDown():
                test_passed = False
        except Exception as inst:
            self.log.error("Verify Redis is down caused exception:")
            self.log.error(traceback.format_exc())
            test_passed = False

        super(self.__class__, self).reportTestCaseResult(test_passed)

        # If there is any test fixture teardown to be done, do it here.

        if self.parameters["pyUnit"]:
            self.assertTrue(test_passed)
        else:
            return test_passed


    def test_VerifyRedisDown(self):
        """
        Test Case:
        Attempt to verify Redis is Down.
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]

        # TODO(Greg): Probably in the config file we need to indicate by node whether it is
        # based on an install or a development environment. For now, we check for an install
        # directory structure for Redis support. If not there, we assume a development environment.
        if fdscfg.rt_env.env_install:
            sbin_dir = fdscfg.rt_env.get_sbin_dir()
        else:
            sbin_dir = os.path.join(fdscfg.rt_env.get_fds_source(), 'tools')

        # If we were passed a node, use it and get out. Otherwise,
        # we use the one captured as the OM node during config parsing.
        if self.passedNode is not None:
            n = self.passedNode
        else:
            n = fdscfg.rt_om_node

        self.log.info("Verify Redis is down node %s." % n.nd_conf_dict['node-name'])

        # Parameter return_stdin is set to return stdout. ... Don't ask me!
        status, stdout = n.nd_agent.exec_wait("%s/redis.sh status" % sbin_dir, return_stdin=True)

        if status != 0:
            self.log.error("Verify Redis is down on node %s returned status %d." % (n.nd_conf_dict['node-name'], status))
            return False

        self.log.info(stdout)

        if stdout.count("NOT") == 0:
            return False

        return True


if __name__ == '__main__':
    TestCase.FDSTestCase.fdsGetCmdLineConfigs(sys.argv)
    unittest.main()
