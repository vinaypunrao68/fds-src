#!/usr/bin/python
#
# Copyright 2014 by Formation Data Systems, Inc.
#

# FDS test-case pattern requirements.
import unittest
import TestCase

# Module-specific requirements
import sys
import os
import time
import logging
import re
import string

import xmlrunner

import NodeWaitSuite
import NodeVerifyDownSuite
import ClusterBootSuite
import ClusterShutdownSuite

import TestFDSEnvMgt
import TestFDSSysMgt
import TestFDSModMgt
import TestFDSPolMgt
import TestFDSVolMgt
import TestFDSSysVerify


def str_to_obj(astr):
    """
    Using the provided string, locate the Python
    object so identified. For our purposes in the System
    Test framework, we wish to resolve a particular method
    from its qualified string name.
    """

    log = logging.getLogger("str_to_obj")

    log.info('Resolving %s.' % astr)

    try:
        return globals()[astr]
    except KeyError:
        try:
            __import__(astr)
            mod = sys.modules[astr]
            return mod
        except ImportError:
            module, _, basename = astr.rpartition('.')
            if module:
                mod = str_to_obj(module)
                return getattr(mod, basename)
            else:
                raise Exception


def queue_up_scenario(suite, scenario, log_dir=None):
    """
    Given the scenario, queue one or more test cases,
    depending upon the associated script and action,
    onto the given test suite.
    """
    log = logging.getLogger("queue_up_scenario")

    # The "script" config for the scenario implies the
    # test case(s) to be executed.
    script = scenario.nd_conf_dict['script']

    # Has the scenario indicated that some sort of delay
    # or wait should follow?
    #
    # Note that given the script,
    # either delay or wait may be ignored even if specified
    # because one or the other or neither make sense for the
    # scenario. At least, that's how it works now.
    if 'delay_wait' in scenario.nd_conf_dict:
        delay = int(scenario.nd_conf_dict['delay_wait'])
    else:
        delay = 0

    # Intended to cause the test to delay until the scenario (which may
    # be on a separate thread of process) completes.
    # Not used currently.
    #if 'wait_completion' in scenario.nd_conf_dict:
    #    wait = scenario.nd_conf_dict['wait_completion']
    #else:
    #    wait = 'false'

    # Based on the script defined in the scenario, take appropriate
    # action which typically includes executing one or more test cases.

    # Based on the script defined in the scenario, take appropriate
    # action which typically includes executing one or more test cases.
    if re.match('\[cluster\]', script) is not None:
        # What action should be taken against the cluster? If not stated, assume "install-boot-activate".
        if "action" in scenario.nd_conf_dict:
            action = scenario.nd_conf_dict['action']
        else:
            action = "install-boot-activate"

        if (action.count("install") > 0) or (action.count("boot") > 0) or (action.count("activate") > 0):
            # Start this cluster as indicated by the action.
            clusterBootSuite = ClusterBootSuite.suiteConstruction(self=None, action=action)
            suite.addTest(clusterBootSuite)
        elif (action.count("remove") > 0) or (action.count("shutdown") > 0) or (action.count("kill") > 0) or\
                (action.count("uninst") > 0):
            # Shutdown the cluster as indicated by the action.
            clusterShutdownSuite = ClusterShutdownSuite.suiteConstruction(self=None, action=action)
            suite.addTest(clusterShutdownSuite)
        else:
            log.error("Unrecognized cluster action '%s' for scenario %s" %
                      (action, scenario.nd_conf_dict['scenario-name']))
            raise Exception

    elif re.match('\[node.+\]', script) is not None:
        # What action should be taken against the node? If not stated, assume "install-boot".
        if "action" in scenario.nd_conf_dict:
            action = scenario.nd_conf_dict['action']
        else:
            action = "install-boot"

        if (action.count("install") > 0) or (action.count("boot") > 0) or (action.count("activate") > 0):
            # Start this node according to the specified action.
            found = False
            for node in scenario.cfg_sect_nodes:
                if '[' + node.nd_conf_dict['node-name'] + ']' == script:
                    found = True

                    if (action.count("install") > 0):
                        # First, build out the installation directory and start Redis.
                        suite.addTest(TestFDSEnvMgt.TestFDSCreateInstDir(node=node))

                        # Boot Redis on the machine if requested.
                        if 'redis' in node.nd_conf_dict:
                            if node.nd_conf_dict['redis'] == 'true':
                                suite.addTest(TestFDSEnvMgt.TestRestartRedisClean(node=node))

                    if (action.count("boot") > 0):
                        # Now bring up PM.
                        suite.addTest(TestFDSModMgt.TestPMBringUp(node=node))
                        suite.addTest(TestFDSModMgt.TestPMWait(node=node))

                        # If the node also supports an OM, start the OM
                        # as well.
                        if node.nd_run_om():
                            suite.addTest(TestFDSModMgt.TestOMBringUp(node=node))
                            suite.addTest(TestFDSModMgt.TestOMWait(node=node))
                            suite.addTest(TestWait(delay=10, reason="to let OM initialize"))

                    if (action.count("activate") > 0):
                        # Now activate the node's configured services.
                        suite.addTest(TestFDSSysMgt.TestNodeActivate(node=node))
                        suite.addTest(TestWait(delay=10, reason="to let the node activate"))

                    break

            if found:
                # Give the node some time to initialize if requested.
                if 'delay_wait' in scenario.nd_conf_dict:
                    suite.addTest(TestWait(delay=delay,
                                                             reason="to allow node " + script + " to initialize"))
            else:
                log.error("Node not found for scenario '%s'" %
                          (scenario.nd_conf_dict['scenario-name']))
                raise Exception
        elif (action.count("remove") > 0) or (action.count("kill") > 0) or (action.count("uninst") > 0):
            # Shutdown the node according to the specified action.
            found = False
            for node in scenario.cfg_sect_nodes:
                if '[' + node.nd_conf_dict['node-name'] + ']' == script:
                    found = True
                    if (action.count("remove") > 0):
                        suite.addTest(TestFDSSysMgt.TestNodeRemoveServices(node=node))

                    if (action.count("kill") > 0):
                        suite.addTest(TestFDSSysMgt.TestNodeKill(node=node))

                    if (action.count("uninst") > 0):
                        suite.addTest(TestFDSEnvMgt.TestFDSDeleteInstDir(node=node))

                        # Shutdown Redis on the machine if we started it.
                        if 'redis' in node.nd_conf_dict:
                            if node.nd_conf_dict['redis'] == 'true':
                                suite.addTest(TestFDSEnvMgt.TestShutdownRedis(node=node))

                    break

            if found:
                # Give the cluster some time to reinitialize if requested.
                if 'delay_wait' in scenario.nd_conf_dict:
                    suite.addTest(TestWait(delay=delay,
                                                             reason="to allow cluster " + script + " to reinitialize"))
            else:
                log.error("Node not found for scenario '%s'" %
                          (scenario.nd_conf_dict['scenario-name']))
                raise Exception

        # Not sure this one has any usefulness.
        elif action == "cleaninst":
            # Selectively clean up the installation area built during bootup.
            found = False
            for node in scenario.cfg_sect_nodes:
                if '[' + node.nd_conf_dict['node-name'] + ']' == script:
                    found = True
                    suite.addTest(TestFDSEnvMgt.TestFDSSelectiveInstDirClean(node=node))
                    break

            if not found:
                log.error("Node not found for scenario '%s'" %
                          (scenario.nd_conf_dict['scenario-name']))
                raise Exception
        else:
            log.error("Unrecognized node action '%s' for scenario %s" %
                      (action, scenario.nd_conf_dict['scenario-name']))
            raise Exception

    elif re.match('\[service\]', script) is not None:
        # What action should be taken against the service? If not stated, assume "boot".
        if "action" in scenario.nd_conf_dict:
            action = scenario.nd_conf_dict['action']
        else:
            action = "boot"

        # Find the node in question
        node = None
        if "fds_node" in scenario.nd_conf_dict:
            found = False
            for node in scenario.cfg_sect_nodes:
                if node.nd_conf_dict['node-name'] == scenario.nd_conf_dict['fds_node']:
                    found = True
                    break

            if not found:
                log.error("Node not found for scenario '%s'" %
                          (scenario.nd_conf_dict['scenario-name']))
                raise Exception
        else:
            log.error("fds_node option not found for scenario '%s'" %
                      (scenario.nd_conf_dict['scenario-name']))
            raise Exception

        # Validate the service in question
        if "service" in scenario.nd_conf_dict:
            if scenario.nd_conf_dict['service'] not in ['pm', 'dm', 'sm', 'am', 'om']:
                log.error("Service %s is not valid for scenario '%s'." %
                          (scenario.nd_conf_dict['service'], scenario.nd_conf_dict['scenario-name']))
                raise Exception
        else:
            log.error("service option not found for scenario '%s'" %
                      (scenario.nd_conf_dict['scenario-name']))
            raise Exception

        if (action.count("install") > 0) or (action.count("boot") > 0) or (action.count("activate") > 0):
            # Start this service according to the specified action.
            if (action.count("install") > 0):
                # First, build out the installation directory and start Redis.
                suite.addTest(TestFDSEnvMgt.TestFDSCreateInstDir(node=node))
                # Boot Redis on the machine if requested.
                if 'redis' in node.nd_conf_dict:
                    if node.nd_conf_dict['redis'] == 'true':
                        suite.addTest(TestFDSEnvMgt.TestRestartRedisClean(node=node))

            if (action.count("boot") > 0):
                # Now bring up PM.
                suite.addTest(TestFDSModMgt.TestPMBringUp(node=node))
                suite.addTest(TestFDSModMgt.TestPMWait(node=node))

                # If the node also supports an OM, start the OM
                # as well.
                if node.nd_run_om():
                    suite.addTest(TestFDSModMgt.TestOMBringUp(node=node))
                    suite.addTest(TestFDSModMgt.TestOMWait(node=node))

            if (action.count("activate") > 0):
                # Now activate the node's configured services.
                suite.addTest(TestFDSSysMgt.TestNodeActivate(node=node))
                suite.addTest(TestWait(delay=10, reason="to let the node activate"))

            # Give the node some time to initialize if requested.
            if 'delay_wait' in scenario.nd_conf_dict:
                suite.addTest(TestWait(delay=delay,
                                                             reason="to allow node " + script + " to initialize"))
        elif (action.count("remove") > 0) or (action.count("kill") > 0) or (action.count("uninst") > 0):
            # Shutdown the node according to the specified action.
            if (action.count("remove") > 0):
                suite.addTest(TestFDSSysMgt.TestNodeRemoveServices(node=node))

            if (action.count("kill") > 0):
                suite.addTest(TestFDSSysMgt.TestNodeKill(node=node))

            if (action.count("uninst") > 0):
                suite.addTest(TestFDSEnvMgt.TestFDSDeleteInstDir(node=node))

                # Shutdown Redis on the machine if we started it.
                if 'redis' in node.nd_conf_dict:
                    if node.nd_conf_dict['redis'] == 'true':
                        suite.addTest(TestFDSEnvMgt.TestShutdownRedis(node=node))

    # We'll leave this here in case a need arises to treat AM's separately (for example
    # booting them on nodes without a PM), but for now AM sections will be flagged.
    elif re.match('\[am.+\]', script) is not None:
            log.error("For scenario %s, don't use AM sections. Instead, specify 'am<,and other services>' in "
                      "the node's definition with a 'services' configuration. By default you get 'services=am,dm,sm'." %
                      (scenario.nd_conf_dict['scenario-name']))
            raise Exception
#            # What action should be taken against the AM? If not stated, assume "activate".
#            if "action" in scenario.nd_conf_dict:
#                action = scenario.nd_conf_dict['action']
#            else:
#                action = "activate"
#
#            if action == "activate":
#                suite.addTest(TestFDSModMgt.TestAMActivate())
#
#                # Give the AM some time to initialize if requested.
#                if 'delay_wait' in scenario.nd_conf_dict:
#                    suite.addTest(TestWait(delay=delay,
#                                                             reason="to allow AM " + script + " to initialize"))
#            elif action == "bootup":
#                found = False
#                for am in scenario.cfg_sect_ams:
#                    if '[' + am.nd_conf_dict['am-name'] + ']' == script:
#                        found = True
#                        suite.addTest(TestFDSModMgt.TestAMBringup(am=am))
#                        break
#
#                if found:
#                    # Give the AM some time to initialize if requested.
#                    if 'delay_wait' in scenario.nd_conf_dict:
#                        suite.addTest(TestWait(delay=delay,
#                                                                 reason="to allow AM " + script + " to initialize"))
#                else:
#                    log.error("AM not found for scenario '%s'" %
#                              (scenario.nd_conf_dict['scenario-name']))
#                    raise Exception
#            elif action == "shutdown":
#                pass
#            else:
#                log.error("Unrecognized node action '%s' for scenario %s" %
#                          (action, scenario.nd_conf_dict['scenario-name']))
#                raise Exception

    elif re.match('\[verify\]', script) is not None:
        # What state should be verified for the given nodes? If not stated, assume "up".
        if "state" in scenario.nd_conf_dict:
            state = scenario.nd_conf_dict['state']
        else:
            state = "up"

        if state == "up":
            # Check that the specified node(s) is(are) up. If no node
            # specified, check that all defined are up.
            if "fds_nodes" in scenario.nd_conf_dict:
                fdsNodeNames = scenario.nd_conf_dict['fds_nodes'].split(",")
                fdsNodes = []
                for node in scenario.cfg_sect_nodes:
                    if node.nd_conf_dict['node-name'] in fdsNodeNames:
                        fdsNodes.append(node)
            else:
                fdsNodes = None

            nodeUpSuite = NodeWaitSuite.suiteConstruction(self=None, fdsNodes=fdsNodes)
            suite.addTest(nodeUpSuite)
        elif state == "down":
            # Check that the specified node(s) is(are) down. If no node
            # specified, check that all defined are down.
            if "fds_nodes" in scenario.nd_conf_dict:
                fdsNodeNames = scenario.nd_conf_dict['fds_nodes'].split(",")
                fdsNodes = []
                for node in scenario.cfg_sect_nodes:
                    if node.nd_conf_dict['node-name'] in fdsNodeNames:
                        fdsNodes.append(node)
            else:
                fdsNodes = None

            nodeDownSuite = NodeVerifyDownSuite.suiteConstruction(self=None, fdsNodes=fdsNodes)
            suite.addTest(nodeDownSuite)
        else:
            log.error("Unrecognized node state '%s' for scenario %s" %
                      (state, scenario.nd_conf_dict['scenario-name']))
            raise Exception

    elif re.match('\[policy.+\]', script) is not None:
        # What action should be taken against the volume policy? If not stated, assume "create".
        if "action" in scenario.nd_conf_dict:
            action = scenario.nd_conf_dict['action']
        else:
            action = "create"

        if action == "create":
            # Set the specified volume policy.
            found = False
            for policy in scenario.cfg_sect_vol_pol:
                if '[' + policy.nd_conf_dict['pol-name'] + ']' == script:
                    found = True
                    suite.addTest(TestFDSPolMgt.TestPolicyCreate(policy=policy))
                    break

            if found:
                # Give the volume policy some time to propagate if requested.
                if 'delay_wait' in scenario.nd_conf_dict:
                    suite.addTest(TestWait(delay=delay,
                                                             reason="to allow volume policy create " + script + " to propagate"))
            else:
                log.error("Volume policy not found for scenario '%s'" %
                          (scenario.nd_conf_dict['scenario-name']))
                raise Exception
        elif action == "delete":
            # Set the specified volume policy.
            found = False
            for policy in scenario.cfg_sect_vol_pol:
                if '[' + policy.nd_conf_dict['pol-name'] + ']' == script:
                    found = True
                    suite.addTest(TestFDSPolMgt.TestPolicyDelete(policy=policy))
                    break

            if found:
                # Give the volume policy delete some time to propagate if requested.
                if 'delay_wait' in scenario.nd_conf_dict:
                    suite.addTest(TestWait(delay=delay,
                                                             reason="to allow volume policy delete " + script + " to propagate"))
            else:
                log.error("Volume policy not found for scenario '%s'" %
                          (scenario.nd_conf_dict['scenario-name']))
                raise Exception
        else:
            log.error("Unrecognized node action '%s' for scenario %s" %
                      (action, scenario.nd_conf_dict['scenario-name']))
            raise Exception

    elif re.match('\[volume.+\]', script) is not None:
        # What action should be taken against the volume? If not stated, assume "create".
        if "action" in scenario.nd_conf_dict:
            action = scenario.nd_conf_dict['action']
        else:
            action = "create"

        if action == "create":
            found = False
            for volume in scenario.cfg_sect_volumes:
                if '[' + volume.nd_conf_dict['vol-name'] + ']' == script:
                    found = True
                    suite.addTest(TestFDSVolMgt.TestVolumeCreate(volume=volume))
                    break

            if found:
                # Give the volume creation some time to propagate if requested.
                if 'delay_wait' in scenario.nd_conf_dict:
                    suite.addTest(TestWait(delay=delay,
                                                             reason="to allow volume creation " + script + " to propagate"))
            else:
                log.error("Volume not found for scenario '%s'" %
                          (scenario.nd_conf_dict['scenario-name']))
                raise Exception
        elif action == "attach":
            found = False
            for volume in scenario.cfg_sect_volumes:
                if '[' + volume.nd_conf_dict['vol-name'] + ']' == script:
                    found = True
                    suite.addTest(TestFDSVolMgt.TestVolumeAttach(volume=volume))
                    break

            if found:
                # Give the volume attachment some time to propagate if requested.
                if 'delay_wait' in scenario.nd_conf_dict:
                    suite.addTest(TestWait(delay=delay,
                                                             reason="to allow volume attachment " + script + " to propagate"))
            else:
                log.error("Volume not found for scenario '%s'" %
                          (scenario.nd_conf_dict['scenario-name']))
                raise Exception
        elif action == "delete":
            found = False
            for volume in scenario.cfg_sect_volumes:
                if '[' + volume.nd_conf_dict['vol-name'] + ']' == script:
                    found = True
                    suite.addTest(TestFDSVolMgt.TestVolumeDelete(volume=volume))
                    break

            if found:
                # Give the volume deletion some time to propagate if requested.
                if 'delay_wait' in scenario.nd_conf_dict:
                    suite.addTest(TestWait(delay=delay,
                                                             reason="to allow volume deletion " + script + " to propagate"))
            else:
                log.error("Volume not found for scenario '%s'" %
                          (scenario.nd_conf_dict['scenario-name']))
                raise Exception
        else:
            log.error("Unrecognized node action '%s' for scenario %s" %
                      (action, scenario.nd_conf_dict['scenario-name']))
            raise Exception

    elif re.match('\[waitforlog\]', script) is not None:
        # Verify the section.
        if ('fds_node' not in scenario.nd_conf_dict) or \
            ('service' not in scenario.nd_conf_dict) or \
            ('logentry' not in scenario.nd_conf_dict) or \
            ('occurrences' not in scenario.nd_conf_dict) or \
            ('maxwait' not in scenario.nd_conf_dict):
            log.error("Scenario section %s is missing one of 'fds_node', 'service', 'logentry', 'occurrences', or 'maxwait'" %
                      (scenario.nd_conf_dict['scenario-name']))
            raise Exception
        elif scenario.nd_conf_dict['service'] not in ["om", "am", "sm", "dm", "pm"]:
            log.error("Scenario section %s service config identifies an invalid service, %s." %
                      (scenario.nd_conf_dict['scenario-name'], scenario.nd_conf_dict['service']))
            raise Exception
        elif len(scenario.nd_conf_dict['logentry']) == 0:
            log.error("Scenario section %s logentry config identifies an empty string." %
                      (scenario.nd_conf_dict['scenario-name']))
            raise Exception
        else:
            occurrences = int(scenario.nd_conf_dict['occurrences'])
            maxwait = int(scenario.nd_conf_dict['maxwait'])

        # Locate the node.
        found = False
        n = None
        for node in scenario.cfg_sect_nodes:
            if node.nd_conf_dict['node-name'] == scenario.nd_conf_dict['fds_node']:
                found = True
                suite.addTest(TestFDSSysVerify.TestWaitForLog(node=node, service=scenario.nd_conf_dict['service'],
                                                                        logentry=scenario.nd_conf_dict['logentry'],
                                                                        occurrences=occurrences, maxwait=maxwait))
                break

        if found:
            # Give the test some time if requested.
            if 'delay_wait' in scenario.nd_conf_dict:
                suite.addTest(TestWait(delay=delay,
                                                         reason="just because"))
        else:
            log.error("Node not found for scenario '%s'" %
                      (scenario.nd_conf_dict['scenario-name']))
            raise Exception

    elif re.match('\[canonmatch\]', script) is not None:
        # Verify the section.
        if ('canon' not in scenario.nd_conf_dict) or \
           ('filetocheck' not in scenario.nd_conf_dict):
            log.error("Scenario section %s is missing one of 'canon', or 'filetocheck'" %
                      (scenario.nd_conf_dict['scenario-name']))
            raise Exception

        suite.addTest(TestFDSSysVerify.TestCanonMatch(canon=scenario.nd_conf_dict['canon'],
                                                                fileToCheck=scenario.nd_conf_dict['filetocheck']))

        # Give the test some time if requested.
        if 'delay_wait' in scenario.nd_conf_dict:
            suite.addTest(TestWait(delay=delay,
                                                     reason="just because"))

    elif re.match('\[testcases.+\]', script) is not None:
        # Do we have any parameters?
        param_names = None
        params = None
        if 'param_names' in scenario.nd_conf_dict:
            param_names = scenario.nd_conf_dict['param_names'].split(",")
            if 'params' in scenario.nd_conf_dict:
                params = scenario.nd_conf_dict['params'].split(",")
                if len(param_names) != len(params):
                    log.error("Mis-matched parameters and parameter values in scenario %s" %
                              (scenario.nd_conf_dict['scenario-name']))
                    log.error(param_names)
                    log.error(params)
                    raise Exception
            else:
                log.error("Missing parameter values for parameters in scenario %s" %
                          (scenario.nd_conf_dict['scenario-name']))
                log.error(param_names)
                raise Exception
        else:
            param_names = None

        # Test cases are "forkable".
        if 'fork' in scenario.nd_conf_dict:
                if param_names is None:
                    param_names = ["fork"]
                    params = [scenario.nd_conf_dict["fork"]]
                else:
                    param_names.add("fork")
                    params.add(scenario.nd_conf_dict["fork"])

        if param_names is not None:
            # Build parameter dictionary.
            kwargs = dict(zip(param_names, params))
            testcase = str_to_obj(script.strip('[]'))(**kwargs)
        else:
            testcase = str_to_obj(script.strip('[]'))(None)

        try:
            suite.addTest(testcase)
        except Exception:
            log.error("Unrecognized test case '%s' for scenario %s" %
                      (script, scenario.nd_conf_dict['scenario-name']))
            raise Exception

        # Give the test case some time to complete async activities if requested.
        if 'delay_wait' in scenario.nd_conf_dict:
            suite.addTest(TestWait(delay=delay, reason="to allow async work from test case " + script + " to complete"))

    elif re.match('\[testsuite.+\]', script) is not None:
        testsuite_name = string.split(script.strip('[]'), '.')[1] + ".suiteConstruction"
        testsuite_obj = str_to_obj(testsuite_name)
        testsuite = testsuite_obj(self=None)

        try:
            suite.addTest(testsuite)
        except Exception:
            log.error("Unrecognized test suite '%s' for scenario %s" %
                      (script, scenario.nd_conf_dict['scenario-name']))
            raise Exception

        # Give the test case some time to complete async activities if requested.
        if 'delay_wait' in scenario.nd_conf_dict:
            suite.addTest(TestWait(delay=delay, reason="to allow async work from test suite " + script + " to complete"))

    elif re.match('\[fork\]', script) is not None:
        # This script indicates that a separate process should be
        # forked to execute the "real" script.

        # Verify we have the "real" script.
        if 'real-script' not in scenario.nd_conf_dict:
            log.error("Scenario section %s is missing option 'real-script'." %
                      (scenario.nd_conf_dict['scenario-name']))
            raise Exception

        suite.addTest(TestFork(scenario=scenario, log_dir=log_dir))

    else:
        log.error("Unrecognized script '%s' for scenario %s" %
                  (script, scenario.nd_conf_dict['scenario-name']))
        raise Exception

    # Currently unsupported FDS scenario config file scenarios.
    #elif re.match('\[fdscli.*\]', script) != None:
    #    # Execute the FDS console commands.
    #    for s in scenario.cfg_sect_cli:
    #        if '[' + s.nd_conf_dict['cli-name'] + ']' == script:
    #            s.run_cli(scenario.nd_conf_dict['script_args'])
    #            break
        # Delay the test if requested.
    #    if 'delay_wait' in scenario.nd_conf_dict:
    #        suite.addTest(TestFDSSysMgt.TestWait(delay=delay))
    #else:
    #    if 'script_args' in self.nd_conf_dict:
    #        script_args = self.nd_conf_dict['script_args']
    #    else:
    #        script_args = ''
    #    print "Running external script: ", script, script_args
    #    args = script_args.split()
    #    p1 = subprocess.Popen([script] + args)
    #    if wait == 'true':
    #        p1.wait()


# This class contains the attributes and methods to give
# delay the current thread a specified number of seconds.
class TestWait(TestCase.FDSTestCase):
    def __init__(self, parameters=None, delay=None, reason=None):
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_Wait,
                                             "Wait")

        if delay is not None:
            if isinstance(delay, int):
                self.passedDelay = delay
            else:
                self.passedDelay = int(delay)
        else:
            self.passedDelay = delay

        self.passedReason = reason

    def test_Wait(self):
        """
        Test Case:
        Cause the test run to sleep for a specified number of seconds.
        """

        if self.passedDelay is not None:
            delay = self.passedDelay
        else:
            delay = 20

        if self.passedReason is not None:
            reason = self.passedReason
        else:
            reason = "just because"

        self.log.info("Waiting %d seconds %s." % (delay, reason))
        time.sleep(delay)

        return True


# This class contains the attributes and methods to
# run the specified object (test case, test suite, or
# scenario script) in a separate process.
class TestFork(TestCase.FDSTestCase):
    def __init__(self, parameters=None, scenario=None, log_dir=None):
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_Fork,
                                             "Fork",
                                             fork=True)

        self.passedScenario = scenario
        self.passedLogDir = log_dir

    def test_Fork(self):
        """
        Test Case:
        Set up the scenario to run in a forked process.
        """

        self.childPID = os.fork()

        if self.childPID == 0:
            self.log.info("Child process executing scenario %s." % self.passedScenario.nd_conf_dict['scenario-name'])
        elif self.childPID > 0:
            self.log.info("Forked child %d to execute scenario %s." % (self.childPID,
                                                                       self.passedScenario.nd_conf_dict['scenario-name']))
            return True
        else:
            self.log.error("Failed to fork a child process to execute scenario %s." %
                           self.passedScenario.nd_conf_dict['scenario-name'])
            return False

        test_suite = unittest.TestSuite()

        # If the "real" script is "scenario", then we have to reconstruct
        # our scenarios based upon the specified FDS Scenario config.
        if self.passedScenario.nd_conf_dict['real-script'] == "scenario":
            # Verify we have a "fds-scenario" option.
            if "fds-scenario" not in self.passedScenario.nd_conf_dict:
                self.log.error("Missing option 'fds-scenario', a path to an FDS Scenario config file, for scenario %s." %
                               self.passedScenario.nd_conf_dict['scenario-name'])
                return False

            self.log.info("Building a test suite based upon FDS Scenario config file %s." %
                           self.passedScenario.nd_conf_dict['scenario-name'])

            # Reset the scenario section based upon the contents of the
            # specified FDS Scenario config.
            fdscfg = self.parameters["fdscfg"]
            fdscfg.rt_obj.cfg_file = self.passedScenario.nd_conf_dict['fds-scenario']
            fdscfg.rt_obj.config_parse_scenario()

            # Map the new scenarios to test cases.
            scenarios = fdscfg.rt_get_obj('cfg_scenarios')

            # If we don't have any scenarios someone screwed up
            # the FDS config file.
            if len(scenarios) == 0:
                self.log.error("Your FDS config file, %s, must specify scenario sections." %
                          self.passedScenario.nd_conf_dict['fds-scenario'])
                sys.exit(1)

            for scenario in scenarios:
                queue_up_scenario(suite=test_suite, scenario=scenario, log_dir=self.passedLogDir)
        else:
            self.log.info("Building a test suite based upon 'real' script %s." %
                           self.passedScenario.nd_conf_dict['real-script'])

            # Set the "real" script for the scenario and queue it up.
            self.passedScenario.nd_conf_dict['script'] = self.passedScenario.nd_conf_dict['real-script']
            queue_up_scenario(suite=test_suite, scenario=self.passedScenario)

        self.log.info("Starting an independent test suite for scenario %s." %
                       self.passedScenario.nd_conf_dict['scenario-name'])

        # Now run the test suite.
        runner = xmlrunner.XMLTestRunner(output=self.passedLogDir)
        runner.run(test_suite)

        return True


if __name__ == '__main__':
    TestCase.FDSTestCase.fdsGetCmdLineConfigs(sys.argv)
    unittest.main()