#!/usr/bin/python
#
# Copyright 2015 by Formation Data Systems, Inc.
#
# This test suite dynamically constructs itself (that is,
# identifies the test cases from which a suite is derived)
# from "scenario" sections in the provided FDS
# configuration file. Therefore, those test cases that it
# runs are completely determined by the FDS configuration
# file.
#

import sys
import unittest
import xmlrunner
import testcases.TestCase
import testcases.TestFDSEnvMgt
import testcases.TestFDSModMgt
import testcases.TestFDSSysMgt
import testcases.TestFDSSysLoad
import testcases.TestFDSSysVerify
import testcases.TestFDSPolMgt
import testcases.TestFDSVolMgt
import testcases.TestMgt
import NodeWaitSuite
import ClusterBootSuite
import ClusterShutdownSuite
import logging
import re
import string


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
            mod=sys.modules[astr]
            return mod
        except ImportError:
            module,_,basename=astr.rpartition('.')
            if module:
                mod=str_to_obj(module)
                return getattr(mod,basename)
            else:
                raise Exception

def suiteConstruction(self):
    """
    Construct the ordered set of test cases that comprise the
    test suite defined by the input FDS scenario config file.
    """
    suite = unittest.TestSuite()

    # By constructing a generic test case at this point, we gain
    # access to the FDS config file from which
    # we'll pull the scenarios used to identify test cases.
    genericTestCase = testcases.TestCase.FDSTestCase()
    fdscfg = genericTestCase.parameters["fdscfg"]
    log = logging.getLogger("ScenarioDriverSuite")

    # Pull the scenarios from the FDS config file. These
    # will be mapped to test cases.
    scenarios = fdscfg.rt_get_obj('cfg_scenarios')

    # If we don't have any scenarios someone screwed up
    # the FDS config file.
    if len(scenarios) == 0:
        log.error("Your FDS config file, %s, must specify scenario sections." %
                  genericTestCase.parameters["fds_config_file"])
        sys.exit(1)

    for scenario in scenarios:
        # The "script" config for this scenario implies the
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

        if 'wait_completion' in scenario.nd_conf_dict:
            wait = scenario.nd_conf_dict['wait_completion']
        else:
            wait = 'false'

        # Based on the script defined in the scenario, take appropriate
        # action which typically includes executing one or more test cases.
        if re.match('\[node.+\]', script) is not None:
            # What action should be taken against the node? If not stated, assume "bootup".
            if "action" in scenario.nd_conf_dict:
                action = scenario.nd_conf_dict['action']
            else:
                action = "bootup"

            if action == "bootup":
                # Start this node. That is, start it's Platform Manager.
                found = False
                for node in scenario.cfg_sect_nodes:
                    if '[' + node.nd_conf_dict['node-name'] + ']' == script:
                        found = True

                        # First, build out the installation directory and start Redis.
                        suite.addTest(testcases.TestFDSEnvMgt.TestFDSCreateInstDir(node=node,
                                                                                   nodeID=node.nd_nodeID))

                        # Boot Redis on the machine if requested.
                        if 'redis' in node.nd_conf_dict:
                            if node.nd_conf_dict['redis'] == 'true':
                                suite.addTest(testcases.TestFDSEnvMgt.TestRestartRedisClean(node=node))

                        # Now bring up PM.
                        suite.addTest(testcases.TestFDSModMgt.TestPMBringUp(node=node))
                        suite.addTest(testcases.TestFDSModMgt.TestPMWait(node=node))

                        # If the node also supports an OM, start the OM
                        # as well.
                        if node.nd_run_om():
                            suite.addTest(testcases.TestFDSModMgt.TestOMBringUp(node=node))
                            suite.addTest(testcases.TestFDSModMgt.TestOMWait(node=node))

                        break

                if found:
                    # Give the node some time to initialize if requested.
                    if 'delay_wait' in scenario.nd_conf_dict:
                        suite.addTest(testcases.TestMgt.TestWait(delay=delay,
                                                                 reason="to allow node " + script + " to initialize"))
                else:
                    log.error("Node not found for scenario '%s'" %
                              (scenario.nd_conf_dict['scenario-name']))
                    raise Exception
            elif action == "remove":
                # Remove the node from its cluster.
                found = False
                for node in scenario.cfg_sect_nodes:
                    if '[' + node.nd_conf_dict['node-name'] + ']' == script:
                        found = True
                        suite.addTest(testcases.TestFDSSysMgt.TestNodeRemoveServices(node=node))
                        break

                if found:
                    # Give the cluster some time to reinitialize if requested.
                    if 'delay_wait' in scenario.nd_conf_dict:
                        suite.addTest(testcases.TestMgt.TestWait(delay=delay,
                                                                 reason="to allow cluster " + script + " to reinitialize"))
                else:
                    log.error("Node not found for scenario '%s'" %
                              (scenario.nd_conf_dict['scenario-name']))
                    raise Exception
            elif action == "shutdown":
                # Shutdown the node.
                found = False
                for node in scenario.cfg_sect_nodes:
                    if '[' + node.nd_conf_dict['node-name'] + ']' == script:
                        found = True
                        suite.addTest(testcases.TestFDSSysMgt.TestNodeShutdown(node=node))

                        # Shutdown Redis on the machine if we started it.
                        if 'redis' in node.nd_conf_dict:
                            if node.nd_conf_dict['redis'] == 'true':
                                suite.addTest(testcases.TestFDSEnvMgt.TestShutdownRedis(node=node))

                        break

                if found:
                    # Give the cluster some time to reinitialize if requested.
                    if 'delay_wait' in scenario.nd_conf_dict:
                        suite.addTest(testcases.TestMgt.TestWait(delay=delay,
                                                                 reason="to allow cluster " + script + " to reinitialize"))
                else:
                    log.error("Node not found for scenario '%s'" %
                              (scenario.nd_conf_dict['scenario-name']))
                    raise Exception
            elif action == "deletefds":
                # Delete the installation area built during bootup.
                found = False
                for node in scenario.cfg_sect_nodes:
                    if '[' + node.nd_conf_dict['node-name'] + ']' == script:
                        found = True
                        suite.addTest(testcases.TestFDSEnvMgt.TestFDSDeleteInstDir(node=node))
                        break

                if not found:
                    log.error("Node not found for scenario '%s'" %
                              (scenario.nd_conf_dict['scenario-name']))
                    raise Exception
            elif action == "cleaninst":
                # Selectively clean up the installation area built during bootup.
                found = False
                for node in scenario.cfg_sect_nodes:
                    if '[' + node.nd_conf_dict['node-name'] + ']' == script:
                        found = True
                        suite.addTest(testcases.TestFDSEnvMgt.TestFDSSelectiveInstDirClean(node=node))
                        break

                if not found:
                    log.error("Node not found for scenario '%s'" %
                              (scenario.nd_conf_dict['scenario-name']))
                    raise Exception
            else:
                log.error("Unrecognized node action '%s' for scenario %s" %
                          (action, scenario.nd_conf_dict['scenario-name']))
                raise Exception

        # Based on the script defined in the scenario, take appropriate
        # action which typically includes executing one or more test cases.
        elif re.match('\[cluster\]', script) is not None:
            # What action should be taken against the cluster? If not stated, assume "bootup".
            if "action" in scenario.nd_conf_dict:
                action = scenario.nd_conf_dict['action']
            else:
                action = "bootup"

            if action == "bootup":
                # Start this cluster.
                clusterBootSuite = ClusterBootSuite.suiteConstruction(self=None)
                suite.addTest(clusterBootSuite)
            elif action == "activate":
                # Activate the cluster.
                suite.addTest(testcases.TestFDSSysMgt.TestClusterActivate())

                # Give the activation some time to complete if requested.
                if 'delay_wait' in scenario.nd_conf_dict:
                    suite.addTest(testcases.TestMgt.TestWait(delay=delay,
                                                                   reason="to allow the activation to complete"))
            elif action == "shutdown":
                # Shutdown the cluster.
                clusterShutdownSuite = ClusterShutdownSuite.suiteConstruction(self=None)
                suite.addTest(clusterShutdownSuite)
            else:
                log.error("Unrecognized cluster action '%s' for scenario %s" %
                          (action, scenario.nd_conf_dict['scenario-name']))
                raise Exception

        elif re.match('\[am.+\]', script) is not None:
            # What action should be taken against the AM? If not stated, assume "activate".
            if "action" in scenario.nd_conf_dict:
                action = scenario.nd_conf_dict['action']
            else:
                action = "activate"

            if action == "activate":
                suite.addTest(testcases.TestFDSModMgt.TestAMActivate())

                # Give the AM some time to initialize if requested.
                if 'delay_wait' in scenario.nd_conf_dict:
                    suite.addTest(testcases.TestMgt.TestWait(delay=delay,
                                                             reason="to allow AM " + script + " to initialize"))
            elif action == "bootup":
                found = False
                for am in scenario.cfg_sect_ams:
                    if '[' + am.nd_conf_dict['am-name'] + ']' == script:
                        found = True
                        suite.addTest(testcases.TestFDSModMgt.TestAMBringup(am=am))
                        break

                if found:
                    # Give the AM some time to initialize if requested.
                    if 'delay_wait' in scenario.nd_conf_dict:
                        suite.addTest(testcases.TestMgt.TestWait(delay=delay,
                                                                 reason="to allow AM " + script + " to initialize"))
                else:
                    log.error("AM not found for scenario '%s'" %
                              (scenario.nd_conf_dict['scenario-name']))
                    raise Exception
            elif action == "shutdown":
                pass
            else:
                log.error("Unrecognized node action '%s' for scenario %s" %
                          (action, scenario.nd_conf_dict['scenario-name']))
                raise Exception

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
                pass
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
                        suite.addTest(testcases.TestFDSPolMgt.TestPolicyCreate(policy=policy))
                        break

                if found:
                    # Give the volume policy some time to propagate if requested.
                    if 'delay_wait' in scenario.nd_conf_dict:
                        suite.addTest(testcases.TestMgt.TestWait(delay=delay,
                                                                 reason="to allow volume policy " + script + " to propagate"))
                else:
                    log.error("Volume policy not found for scenario '%s'" %
                              (scenario.nd_conf_dict['scenario-name']))
                    raise Exception
            elif action == "delete":
                pass
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
                        suite.addTest(testcases.TestFDSVolMgt.TestVolumeCreate(volume=volume))
                        break

                if found:
                    # Give the volume creation some time to propagate if requested.
                    if 'delay_wait' in scenario.nd_conf_dict:
                        suite.addTest(testcases.TestMgt.TestWait(delay=delay,
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
                        suite.addTest(testcases.TestFDSVolMgt.TestVolumeAttach(volume=volume))
                        break

                if found:
                    # Give the volume attachment some time to propagate if requested.
                    if 'delay_wait' in scenario.nd_conf_dict:
                        suite.addTest(testcases.TestMgt.TestWait(delay=delay,
                                                                 reason="to allow volume attachment " + script + " to propagate"))
                else:
                    log.error("Volume not found for scenario '%s'" %
                              (scenario.nd_conf_dict['scenario-name']))
                    raise Exception
            elif action == "delete":
                pass
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
                    suite.addTest(testcases.TestFDSSysVerify.TestWaitForLog(node=node, service=scenario.nd_conf_dict['service'],
                                                                            logentry=scenario.nd_conf_dict['logentry'],
                                                                            occurrences=occurrences, maxwait=maxwait))
                    break

            if found:
                # Give the test some time if requested.
                if 'delay_wait' in scenario.nd_conf_dict:
                    suite.addTest(testcases.TestMgt.TestWait(delay=delay,
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

            suite.addTest(testcases.TestFDSSysVerify.TestCanonMatch(canon=scenario.nd_conf_dict['canon'],
                                                                    fileToCheck=scenario.nd_conf_dict['filetocheck']))

            # Give the test some time if requested.
            if 'delay_wait' in scenario.nd_conf_dict:
                suite.addTest(testcases.TestMgt.TestWait(delay=delay,
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
                suite.addTest(testcases.TestMgt.TestWait(delay=delay,
                                                         reason="to allow async work from test case " + script + " to complete"))

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
                suite.addTest(testcases.TestMgt.TestWait(delay=delay,
                                                         reason="to allow async work from test suite " + script + " to complete"))

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
        #        suite.addTest(testcases.TestFDSSysMgt.TestWait(delay=delay))
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


    return suite

if __name__ == '__main__':
	
    # Handle FDS specific commandline arguments.
    log_dir, failfast = testcases.TestCase.FDSTestCase.fdsGetCmdLineConfigs(sys.argv)

    # If a test log directory was not supplied on the command line (with option "-l"),
    # then default it.
    if log_dir is None:
        log_dir = 'test-reports'

    # Get a test runner that will output an xUnit XML report for Jenkins
    # TODO(Greg) I've tried everything I can think of, but stop-on-fail/failfast does not
    # stop the suite upon first test case failure. So I've implemented our own
    # "failfast" via the TestCase.pyUnitTCFailure global. (And besides, some system
    # test users - most notably, Jenkins - were reporting that XMLTestRunner did not
    # recognize the failfast argument.)
    #runner = xmlrunner.XMLTestRunner(output=log_dir, failfast=failfast)
    runner = xmlrunner.XMLTestRunner(output=log_dir)

    test_suite = suiteConstruction(self=None)
    runner.run(test_suite)

