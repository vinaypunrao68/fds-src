#!/usr/bin/python
#
# Copyright 2014 by Formation Data Systems, Inc.
#

# FDS test-case pattern requirements.
import unittest
import TestCase
import pdb

# Module-specific requirements
import sys
import os
import time
import logging
import re
import string
import random
import fdslib.TestUtils as TestUtils

import xmlrunner

import NodeWaitSuite
import NodeVerifyDownSuite
import NodeVerifyShutdownSuite
import DomainBootSuite
import DomainShutdownSuite

import TestFDSEnvMgt
import TestFDSSysMgt
import TestFDSServiceMgt
import TestFDSVolMgt
import TestFDSSysVerify
import NetworkErrorInjection

try:
    # Removed in Python 3
    from cStringIO import StringIO
except ImportError:
    from io import StringIO



class StreamToLogger(object):
   """
   Fake file-like stream object that redirects writes to a logger instance.
   """
   def __init__(self, logger, log_level=logging.INFO):
      self.logger = logger
      self.log_level = log_level
      self.linebuf = ''

   def write(self, buf):
      for line in buf.rstrip().splitlines():
         self.logger.log(self.log_level, line.rstrip())

class _RunnerDelegateIO(object):
    """
    This class defines an object that logs whatever is written to
    a stream or file.
    """

    def __init__(self, delegate, fds_logger, log_level=logging.INFO):
        self.delegate = delegate
        self.stream_to_logger = StreamToLogger(fds_logger, log_level)

    def write(self, text):
        #if hasattr(self.delegate, "write"):
        self.delegate.write(text)
        #else:
        #    self.delegate.info(text)

        self.stream_to_logger.write(text)

class _DelegateIO(object):
    """
    This class defines an object that captures whatever is written to
    a stream or file.
    """

    def __init__(self, delegate, fds_logger, log_level=logging.INFO):
        self._captured = StringIO()
        self.delegate = delegate
        self.stream_to_logger = StreamToLogger(fds_logger, log_level)

    def write(self, text):
        self._captured.write(text)
        self.delegate.write(text)
        self.stream_to_logger.write(text)

    def __getattr__(self, attr):
        return getattr(self._captured, attr)

class FDSTestRunner(xmlrunner.XMLTestRunner):
    """
    A test runner class for FDS so that stdout and stderr are captured in our test logs.
    """
    def __init__(self, output='.', outsuffix=None, stream=sys.stderr,
                 descriptions=True, verbosity=1, elapsed_times=True, fds_logger=logging.getLogger()):
        xmlrunner.XMLTestRunner.__init__(self, output, outsuffix,
                                         _RunnerDelegateIO(stream, fds_logger, log_level=logging.INFO),
                                         descriptions, verbosity, elapsed_times)
        self.fds_logger = fds_logger

    def _patch_standard_output(self):
        """
        Replaces stdout and stderr streams with string-based streams
        in order to capture the tests' output.
        """
        sys.stdout = _DelegateIO(sys.stdout, self.fds_logger, log_level=logging.INFO)
        sys.stderr = _DelegateIO(sys.stderr, self.fds_logger, log_level=logging.ERROR)


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


def queue_up_scenario(suite, scenario, log_dir=None, install_done=None):
    """
    Given the scenario, queue one or more test cases,
    depending upon the associated script and action,
    onto the given test suite.
    """
    log = logging.getLogger("queue_up_scenario")

    # The "script" config for the scenario implies the
    # test case(s) to be executed.
    script = scenario.nd_conf_dict['script']

    # Is there a marker to be logged with the scenario?
    if 'log_marker' in scenario.nd_conf_dict:
        suite.addTest(TestLogMarker(scenario=scenario.nd_conf_dict['scenario-name'],
                                    marker=scenario.nd_conf_dict['log_marker']))

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
    if re.match('\[domain\]', script) is not None:
        # What action should be taken against the domain? If not stated, assume "install-boot-activate".
        if "action" in scenario.nd_conf_dict:
            action = scenario.nd_conf_dict['action']
        else:
            action = "install-boot-activate"

        if (action.count("install") > 0) or (action.count("boot") > 0) or (action.count("activate") > 0) or\
                (action.count("graceful_restart") > 0):
            if install_done:
                log.info("FDS is already installed, hence skippping scenario "+scenario.nd_conf_dict['scenario-name'])
            else:
                # Start this domain as indicated by the action.
                domainBootSuite = DomainBootSuite.suiteConstruction(self=None, action=action)
                suite.addTest(domainBootSuite)
        elif (action.count("remove") > 0) or (action.count("shutdown") > 0) or (action.count("kill") > 0) or\
                (action.count("uninst") > 0):
            # Shutdown the domain as indicated by the action.
            domainShutdownSuite = DomainShutdownSuite.suiteConstruction(self=None, action=action)
            suite.addTest(domainShutdownSuite)
        else:
            log.error("Unrecognized domain action '%s' for scenario %s" %
                      (action, scenario.nd_conf_dict['scenario-name']))
            raise Exception

    elif re.match('\[node.+\]', script) is not None:
        # Support for random node bringup/shutdown
        if "node*" in script:
            # The * will signify random
            if "nodes" in scenario.nd_conf_dict:
                nds = scenario.nd_conf_dict['nodes'].split(',')
                # Add the [ ] around each node name so it comes in the expected format
                nds = map(lambda x: '[' + x + ']', nds)
            else:
                nds = [x for x in scenario.cfg_sect_nodes]
                nds = map(lambda x: x.nd_conf_dict['node-name'], nds)

            # Make random selection
            # Make sure selection size can't be larger than the number of nodes we've got to pick from
            max_size = len(nds) if len(nds) < 4 else 4
            nds = random.sample(nds, random.randint(1, max_size))
            # Track what we picked so we can verify later

        else:
            nds = [script]
        # What action should be taken against the node? If not stated, assume "install-boot".
        if "action" in scenario.nd_conf_dict:
            action = scenario.nd_conf_dict['action']
        else:
            action = "install-boot"

        if (action.count("install") > 0) or (action.count("boot") > 0) or (action.count("activate") > 0):
            # Start this node according to the specified action.
            for script in nds:
                found = False
                for node in scenario.cfg_sect_nodes:
                    if '[' + node.nd_conf_dict['node-name'] + ']' == script:
                        found = True

                        # Mark it as 'selected' so we know which random nodes are up
                        node.selected = True

                        if (action.count("install") > 0):
                            # First, build out the installation directory and start Redis.
                            suite.addTest(TestFDSEnvMgt.TestFDSInstall(node=node))

                            # Boot Redis on the machine if requested.
                            if 'redis' in node.nd_conf_dict:
                                if node.nd_conf_dict['redis'] == 'true':
                                    suite.addTest(TestFDSEnvMgt.TestRestartRedisClean(node=node))

                            # Boot InfluxDB on the machine if requested.
                            if 'influxdb' in node.nd_conf_dict:
                                if node.nd_conf_dict['influxdb'] == 'true':
                                    suite.addTest(TestFDSEnvMgt.TestRestartInfluxDBClean(node=node))

                        if (action.count("boot") > 0):
                            # Now bring up PM.
                            suite.addTest(TestFDSServiceMgt.TestPMBringUp(node=node))
                            suite.addTest(TestFDSServiceMgt.TestPMWait(node=node))

                            # If the node also supports an OM, start the OM
                            # as well.
                            if node.nd_run_om():
                                suite.addTest(TestFDSServiceMgt.TestOMBringUp(node=node))
                                suite.addTest(TestFDSServiceMgt.TestOMWait(node=node))
                                suite.addTest(TestWait(delay=10, reason="to let OM initialize"))

                        if (action.count("activate") > 0):
                            suite.addTest(TestWait(delay=10, reason="to let node initialize before activation"))

                            # Now activate the node's configured services.
                            suite.addTest(TestFDSSysMgt.TestNodeActivate(node=node))
                            suite.addTest(TestWait(delay=10, reason="to let the node activate"))

                        break

                if found:
                    # Give the node some time to initialize if requested.
                    if 'delay_wait' in scenario.nd_conf_dict:
                        suite.addTest(TestWait(delay=delay, reason="to allow node " + script + " to initialize"))
                else:
                    log.error("Node not found for scenario '%s'" %
                              (scenario.nd_conf_dict['scenario-name']))
                    raise Exception

        elif (action.count("remove") > 0) or (action.count("kill") > 0) or (action.count("uninst") > 0):
            # Shutdown the node according to the specified action.
            for script in nds:
                found = False
                for node in scenario.cfg_sect_nodes:
                    if '[' + node.nd_conf_dict['node-name'] + ']' == script:
                        found = True
                        # Prevent scenario where we try to take down/verify a node that was never online
                        if not hasattr(node, 'selected') or node.selected is False:
                            log.info("Selected node {} was never started. Ignoring "
                                     "command to remove/kill/uninstall".format(node.nd_conf_dict['node-name']))
                            continue

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

                            # Shutdown InfluxDB on the machine if we started it.
                            if 'influxdb' in node.nd_conf_dict:
                                if node.nd_conf_dict['influxdb'] == 'true':
                                    suite.addTest(TestFDSEnvMgt.TestShutdownInfluxDB(node=node))

                        break

                if found:
                    # Give the domain some time to reinitialize if requested.
                    if 'delay_wait' in scenario.nd_conf_dict:
                        suite.addTest(TestWait(delay=delay,
                                                                 reason="to allow domain " + script + " to reinitialize"))
                else:
                    log.error("Node not found for scenario '%s'" %
                              (scenario.nd_conf_dict['scenario-name']))
                    raise Exception

        # Not sure this one has any usefulness.
        elif action == "cleaninst":
            # Selectively clean up the installation area built during bootup.
            for script in nds:
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

        # Find the node(s) in question
        node = None
        if "fds_nodes" in scenario.nd_conf_dict:
            if scenario.nd_conf_dict['fds_nodes'] == '*':
                fdsNodeNames = [x for x in scenario.cfg_sect_nodes
                                if hasattr(x, 'selected') and x.selected is True]
            else:
                fdsNodeNames = scenario.nd_conf_dict['fds_nodes'].split(',')

            fdsNodes = []
            for node in scenario.cfg_sect_nodes:
                if node.nd_conf_dict['node-name'] in fdsNodeNames:
                    fdsNodes.append(node)

        elif "fds_node" in scenario.nd_conf_dict:
            found = False
            for node in scenario.cfg_sect_nodes:
                if node.nd_conf_dict['node-name'] == scenario.nd_conf_dict['fds_node']:
                    found = True
                    break

            if not found:
                log.error("Node not found for scenario '%s'" %
                          (scenario.nd_conf_dict['scenario-name']))
                raise Exception

            fdsNodes = None
            
        else:
            # Perform action for given service on all nodes in the domain.
            node = None
            fdsNodes = None

        # Validate the service in question
        selectedServices = None
        if "services" in scenario.nd_conf_dict:
            selectedServices = scenario.nd_conf_dict['services'].split(',')

        elif "service" in scenario.nd_conf_dict:
            if scenario.nd_conf_dict['service'] not in ['pm', 'dm', 'sm', 'om', 'am']:
                log.error("Service %s is not valid for scenario '%s'." %
                          (scenario.nd_conf_dict['service'], scenario.nd_conf_dict['scenario-name']))
                raise Exception
            else:
                service = scenario.nd_conf_dict['service']
        else:
            log.error("Service option not found for scenario '%s'" %
                      (scenario.nd_conf_dict['scenario-name']))
            raise Exception

        if (action.count("boot") > 0):
            if service == "pm":
                suite.addTest(TestFDSServiceMgt.TestPMBringUp(node=node))
            elif service == "dm":
                suite.addTest(TestFDSServiceMgt.TestDMBringUp(node=node))
            elif service == "sm":
                suite.addTest(TestFDSServiceMgt.TestSMBringUp(node=node))
            elif service == "om":
                suite.addTest(TestFDSServiceMgt.TestOMBringUp(node=node))
            elif service == "am":
                suite.addTest(TestFDSServiceMgt.TestAMBringUp(node=node))

        if (action.count("verifyup") > 0):
            if selectedServices == None:
                selectedServices = service.split(',')

            if fdsNodes is not None:
                for node in fdsNodes:
                    for service in selectedServices:
                        if service == "pm":
                            if node is None:
                                # When checking all PM's in the domain, we have to call out the
                                # PM for the OM's node specifically.
                                suite.addTest((TestFDSServiceMgt.TestPMForOMWait(node=node)))
                            suite.addTest(TestFDSServiceMgt.TestPMWait(node=node))
                        elif service == "dm":
                            suite.addTest(TestFDSServiceMgt.TestDMWait(node=node))
                        elif service == "sm":
                            suite.addTest(TestFDSServiceMgt.TestSMWait(node=node))
                        elif service == "om":
                            suite.addTest(TestFDSServiceMgt.TestOMWait(node=node))
                        elif service == "am":
                            suite.addTest(TestFDSServiceMgt.TestAMWait(node=node))

            else:
                for service in selectedServices:
                    if service == "pm":
                        if node is None:
                            # When checking all PM's in the domain, we have to call out the
                            # PM for the OM's node specifically.
                            suite.addTest((TestFDSServiceMgt.TestPMForOMWait(node=node)))
                        suite.addTest(TestFDSServiceMgt.TestPMWait(node=node))
                    elif service == "dm":
                        suite.addTest(TestFDSServiceMgt.TestDMWait(node=node))
                    elif service == "sm":
                        suite.addTest(TestFDSServiceMgt.TestSMWait(node=node))
                    elif service == "om":
                        suite.addTest(TestFDSServiceMgt.TestOMWait(node=node))
                    elif service == "am":
                        suite.addTest(TestFDSServiceMgt.TestAMWait(node=node))

        if (action.count("activate") > 0):
            if service == "pm":
                log.error("Activate action not valid for PM service for scenario '%s'" %
                          (scenario.nd_conf_dict['scenario-name']))
                raise Exception
            elif service == "dm":
                suite.addTest(TestFDSServiceMgt.TestDMActivate(node=node))
            elif service == "sm":
                suite.addTest(TestFDSServiceMgt.TestSMActivate(node=node))
            elif service == "om":
                log.error("Activate action not valid for OM service for scenario '%s'" %
                          (scenario.nd_conf_dict['scenario-name']))
                raise Exception
            elif service == "am":
                suite.addTest(TestFDSServiceMgt.TestAMActivate(node=node))

        if (action.count("remove") > 0):
            if selectedServices == None:
                selectedServices = service.split(',')

            if fdsNodes is not None:
                for node in fdsNodes:
                    for service in selectedServices:
                        if service == "pm":
                            suite.addTest(TestFDSServiceMgt.TestAWSPMRemove(node=node))
                        elif service == "dm":
                            suite.addTest(TestFDSServiceMgt.TestAWSDMRemove(node=node))
                        elif service == "sm":
                            suite.addTest(TestFDSServiceMgt.TestAWSSMRemove(node=node))
                        elif service == "am":
                            suite.addTest(TestFDSServiceMgt.TestAWSAMRemove(node=node))


            else:
                for service in selectedServices:
                    if service == "pm":
                        log.error("Remove action not valid for PM service for scenario '%s'" %
                                  (scenario.nd_conf_dict['scenario-name']))
                        raise Exception
                    elif service == "dm":
                        suite.addTest(TestFDSServiceMgt.TestDMRemove(node=node))
                    elif service == "sm":
                        suite.addTest(TestFDSServiceMgt.TestSMRemove(node=node))
                    elif service == "om":
                        log.error("Remove action not valid for OM service for scenario '%s'" %
                                  (scenario.nd_conf_dict['scenario-name']))
                        raise Exception
                    elif service == "am":
                        suite.addTest(TestFDSServiceMgt.TestAMRemove(node=node))

        if (action.count("add") > 0):
            if selectedServices == None:
                selectedServices = service.split(',')

            if fdsNodes is not None:
                for node in fdsNodes:
                    for service in selectedServices:
                        if service == "pm":
                            suite.addTest(TestFDSServiceMgt.TestAWSPMAdd(node=node))
                        elif service == "dm":
                            suite.addTest(TestFDSServiceMgt.TestAWSDMAdd(node=node))
                        elif service == "sm":
                            suite.addTest(TestFDSServiceMgt.TestAWSSMAdd(node=node))
                        elif service == "am":
                            suite.addTest(TestFDSServiceMgt.TestAWSAMAdd(node=node))
            else:
                for service in selectedServices:
                        if service == "pm":
                            suite.addTest(TestFDSServiceMgt.TestAWSPMAdd(node=node))
                        elif service == "dm":
                            suite.addTest(TestFDSServiceMgt.TestAWSDMAdd(node=node))
                        elif service == "sm":
                            suite.addTest(TestFDSServiceMgt.TestAWSSMAdd(node=node))
                        elif service == "am":
                            suite.addTest(TestFDSServiceMgt.TestAWSAMAdd(node=node))


        if (action.count("kill") > 0):
            if selectedServices == None:
                selectedServices = service.split(',')

            if fdsNodes is not None:
                for node in fdsNodes:
                    for service in selectedServices:
                        if service == "pm":
                            suite.addTest(TestFDSServiceMgt.TestPMKill(node=node))
                        elif service == "dm":
                            suite.addTest(TestFDSServiceMgt.TestDMKill(node=node))
                        elif service == "sm":
                            suite.addTest(TestFDSServiceMgt.TestSMKill(node=node))
                        elif service == "om":
                            suite.addTest(TestFDSServiceMgt.TestOMKill(node=node))
                        elif service == "am":
                            suite.addTest(TestFDSServiceMgt.TestAMKill(node=node))

            else:
                for service in selectedServices:
                        if service == "pm":
                            suite.addTest(TestFDSServiceMgt.TestPMKill(node=node))
                        elif service == "dm":
                            suite.addTest(TestFDSServiceMgt.TestDMKill(node=node))
                        elif service == "sm":
                            suite.addTest(TestFDSServiceMgt.TestSMKill(node=node))
                        elif service == "om":
                            suite.addTest(TestFDSServiceMgt.TestOMKill(node=node))
                        elif service == "am":
                            suite.addTest(TestFDSServiceMgt.TestAMKill(node=node))

        if (action.count("stop") > 0):
            if selectedServices == None:
                selectedServices = service.split(',')

            if fdsNodes is not None:
                for node in fdsNodes:
                    for service in selectedServices:
                        if service == "pm":
                            suite.addTest(TestFDSServiceMgt.TestAWSPMStop(node=node))
                        elif service == "dm":
                            suite.addTest(TestFDSServiceMgt.TestAWSDMStop(node=node))
                        elif service == "sm":
                            suite.addTest(TestFDSServiceMgt.TestAWSSMStop(node=node))
                        elif service == "om":
                            suite.addTest(TestFDSServiceMgt.TestAWSOMStop(node=node))
                        elif service == "am":
                            suite.addTest(TestFDSServiceMgt.TestAWSAMStop(node=node))

            else:
                for service in selectedServices:
                        if service == "pm":
                            suite.addTest(TestFDSServiceMgt.TestAWSPMStop(node=node))
                        elif service == "dm":
                            suite.addTest(TestFDSServiceMgt.TestAWSDMStop(node=node))
                        elif service == "sm":
                            suite.addTest(TestFDSServiceMgt.TestAWSSMStop(node=node))
                        elif service == "om":
                            suite.addTest(TestFDSServiceMgt.TestAWSOMStop(node=node))
                        elif service == "am":
                            suite.addTest(TestFDSServiceMgt.TestAWSAMStop(node=node))


        if (action.count("start") > 0):
            if selectedServices == None:
                selectedServices = service.split(',')

            if fdsNodes is not None:
                for node in fdsNodes:
                    for service in selectedServices:
                        if service == "pm":
                            suite.addTest(TestFDSServiceMgt.TestAWSPMStart(node=node))
                        elif service == "dm":
                            suite.addTest(TestFDSServiceMgt.TestAWSDMStart(node=node))
                        elif service == "sm":
                            suite.addTest(TestFDSServiceMgt.TestAWSSMStart(node=node))
                        elif service == "om":
                            suite.addTest(TestFDSServiceMgt.TestAWSOMStart(node=node))
                        elif service == "am":
                            suite.addTest(TestFDSServiceMgt.TestAWSAMStart(node=node))
            else:
                for service in selectedServices:
                        if service == "pm":
                            suite.addTest(TestFDSServiceMgt.TestAWSPMStart(node=node))
                        elif service == "dm":
                            suite.addTest(TestFDSServiceMgt.TestAWSDMStart(node=node))
                        elif service == "sm":
                            suite.addTest(TestFDSServiceMgt.TestAWSSMStart(node=node))
                        elif service == "om":
                            suite.addTest(TestFDSServiceMgt.TestAWSOMStart(node=node))
                        elif service == "am":
                            suite.addTest(TestFDSServiceMgt.TestAWSAMStart(node=node))


        if (action.count("verifydown") > 0):
            if selectedServices == None:
                selectedServices = service.split(',')

            if fdsNodes is not None:
                for node in fdsNodes:
                    for service in selectedServices:
                        if service == "pm":
                            if node is None:
                                # When checking all PM's in the domain, we have to call out the
                                # PM for the OM's node specifically.
                                suite.addTest((TestFDSServiceMgt.TestPMForOMVerifyDown(node=node)))
                            suite.addTest(TestFDSServiceMgt.TestPMVerifyDown(node=node))
                        elif service == "dm":
                            suite.addTest(TestFDSServiceMgt.TestDMVerifyDown(node=node))
                        elif service == "sm":
                            suite.addTest(TestFDSServiceMgt.TestSMVerifyDown(node=node))
                        elif service == "om":
                            suite.addTest(TestFDSServiceMgt.TestOMVerifyDown(node=node))
                        elif service == "am":
                            suite.addTest(TestFDSServiceMgt.TestAMVerifyDown(node=node))

            else:
                for service in selectedServices:
                        if service == "pm":
                            if node is None:
                                # When checking all PM's in the domain, we have to call out the
                                # PM for the OM's node specifically.
                                suite.addTest((TestFDSServiceMgt.TestPMForOMVerifyDown(node=node)))
                            suite.addTest(TestFDSServiceMgt.TestPMVerifyDown(node=node))
                        elif service == "dm":
                            suite.addTest(TestFDSServiceMgt.TestDMVerifyDown(node=node))
                        elif service == "sm":
                            suite.addTest(TestFDSServiceMgt.TestSMVerifyDown(node=node))
                        elif service == "om":
                            suite.addTest(TestFDSServiceMgt.TestOMVerifyDown(node=node))
                        elif service == "am":
                            suite.addTest(TestFDSServiceMgt.TestAMVerifyDown(node=node))

        # Give the service action some time to complete if requested.
        if 'delay_wait' in scenario.nd_conf_dict:
            suite.addTest(TestWait(delay=delay, reason="to allow " + service + " " + script + " to complete"))

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
#                        suite.addTest(TestFDSModMgt.TestAMBringUp(am=am))
#                        break
#
#                if found:
#                    # Give the AM some time to initialize if requested.
#                    if 'delay_wait' in scenario.nd_conf_dict:
#                        suite.addTest(TestWait(delay=delay, reason="to allow AM " + script + " to initialize"))
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
                if scenario.nd_conf_dict['fds_nodes'] == '*':
                    fdsNodeNames = [x for x in scenario.cfg_sect_nodes
                                    if hasattr(x, 'selected') and x.selected is True]
                else:
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
            # In this case we check that all FDS processes are down typically
            # following a domain or node "kill" action.
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
        elif state == "shutdown":
            # Check that the specified node(s) is(are) shutdown. If no node
            # specified, check that all defined are down.
            # In this case we check that necessary FDS processes are down or up
            # accordingly following a domain or node "shutdown" action.
            if "fds_nodes" in scenario.nd_conf_dict:
                fdsNodeNames = scenario.nd_conf_dict['fds_nodes'].split(",")
                fdsNodes = []
                for node in scenario.cfg_sect_nodes:
                    if node.nd_conf_dict['node-name'] in fdsNodeNames:
                        fdsNodes.append(node)
            else:
                fdsNodes = None

            nodeShutdownSuite = NodeVerifyShutdownSuite.suiteConstruction(self=None, fdsNodes=fdsNodes)
            suite.addTest(nodeShutdownSuite)
        else:
            log.error("Unrecognized node state '%s' for scenario %s" %
                      (state, scenario.nd_conf_dict['scenario-name']))
            raise Exception

    elif re.match('\[policy.+\]', script) is not None:
        log.info(" creating and deleting volume policy is deprecated, QOS policy is passed in creating volume not seperately anymore")
    elif re.match('\[volume.+\]', script) is not None:
        # What action should be taken against the volume? If not stated, assume "create".
        if "action" in scenario.nd_conf_dict:
            action = scenario.nd_conf_dict['action']
        else:
            action = "create"

        if "node" in scenario.nd_conf_dict:
            node = scenario.nd_conf_dict['node']
        else:
            node = None

        if "expect_failure" in scenario.nd_conf_dict:
            expect_to_fail = bool(scenario.nd_conf_dict['expect_failure'])
        else:
            expect_to_fail = False

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
                    suite.addTest(TestWait(delay=delay, reason="to allow volume creation " + script + " to propagate"))
            else:
                log.error("Volume not found for scenario '%s'" %
                          (scenario.nd_conf_dict['scenario-name']))
                raise Exception
        elif action == "attach":
            found = False
            for volume in scenario.cfg_sect_volumes:
                if '[' + volume.nd_conf_dict['vol-name'] + ']' == script:
                    found = True
                    suite.addTest(TestFDSVolMgt.TestVolumeAttach(volume=volume, node=node, expect_to_fail=expect_to_fail))
                    break

            if found:
                # Give the volume attachment some time to propagate if requested.
                if 'delay_wait' in scenario.nd_conf_dict:
                    suite.addTest(TestWait(delay=delay, reason="to allow volume attachment " + script + " to propagate"))
            else:
                log.error("Volume not found for scenario '%s'" %
                          (scenario.nd_conf_dict['scenario-name']))
                raise Exception
        elif action == "detach":
            found = False
            for volume in scenario.cfg_sect_volumes:
                if '[' + volume.nd_conf_dict['vol-name'] + ']' == script:
                    found = True
                    suite.addTest(TestFDSVolMgt.TestVolumeDetach(volume=volume, node=node))
                    break

            if found:
                # Give the volume detachment some time to propagate if requested.
                if 'delay_wait' in scenario.nd_conf_dict:
                    suite.addTest(TestWait(delay=delay, reason="to allow volume detachment " + script + " to propagate"))
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
                    suite.addTest(TestWait(delay=delay, reason="to allow volume deletion " + script + " to propagate"))
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


        if ('atleastone' not in scenario.nd_conf_dict):
            #log.error("%s not found for any occurrence" %(scenario.nd_conf_dict['logentry']))
            atleastone = None
        else:
            atleastone = 1

       # Locate the node.
        found = False
        for node in scenario.cfg_sect_nodes:
            if node.nd_conf_dict['node-name'] == scenario.nd_conf_dict['fds_node']:
                found = True
                suite.addTest(TestFDSSysVerify.TestWaitForLog(node=node, service=scenario.nd_conf_dict['service'],
                                                                        logentry=scenario.nd_conf_dict['logentry'],
                                                                        occurrences=occurrences, maxwait=maxwait,
                                                                        atleastone=atleastone))
                break

        if found:
            # Give the test some time if requested.
            if 'delay_wait' in scenario.nd_conf_dict:
                suite.addTest(TestWait(delay=delay, reason="just because"))
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
            suite.addTest(TestWait(delay=delay, reason="just because"))

    elif re.match('\[testcases.+\]', script) is not None:
        # Do we have any parameters?
        param_names = []
        params = []
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
            param_names = []

        # Test cases are "forkable".
        if 'fork' in scenario.nd_conf_dict:
            param_names.append("fork")
            params.append(scenario.nd_conf_dict["fork"])

        # arg like stuff
        for key,value in scenario.nd_conf_dict.items():
            if key.startswith('arg.'):
                param_names.append(key[4:])
                params.append(value)

        if len(param_names) > 0: 
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

        suite.addTest(TestForkScenario(scenario=scenario, log_dir=log_dir))

    elif re.match('\[kill\]', script) is not None:
        # This script indicates that a previously forked process should be
        # killed.
        suite.addTest(TestKillScenario(scenario=scenario))

    elif re.match('\[join\]', script) is not None:
        # This script indicates that a previously forked process should be
        # joined.
        suite.addTest(TestJoinScenario(scenario=scenario))

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
class TestForkScenario(TestCase.FDSTestCase):
    def __init__(self, parameters=None, scenario=None, log_dir=None):
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_ForkScenario,
                                             "Fork scenario",
                                             fork=True)

        self.passedScenario = scenario
        self.passedLogDir = log_dir

    def test_ForkScenario(self):
        """
        Test Case:
        Set up the scenario to run in a forked process.
        """

        self.childPID = os.fork()

        if self.childPID == 0:
            self.log.info("Child process executing scenario %s." % self.passedScenario.nd_conf_dict['scenario-name'])

            # Restore stdout and stderr streams.
            #
            # Normally, our test runner would do this for us at the conclusion of a test case. But here
            # it does not know that we're starting a new set of test cases.
            sys.stdout = sys.stdout.delegate
            sys.stderr = sys.stderr.delegate

            for h in logging.getLogger().handlers:
                logging.getLogger().removeHandler(h)

            self.log = TestUtils._setup_logging("PyUnit_{}.log".format(self.passedScenario.nd_conf_dict['scenario-name']),
                                                self.parameters["log_dir"], self.parameters["log_level"])

            self.log.info("Child process executing scenario %s." % self.passedScenario.nd_conf_dict['scenario-name'])
        elif self.childPID > 0:
            self.log.info("Forked child %d to execute scenario %s." % (self.childPID,
                                                                       self.passedScenario.nd_conf_dict['scenario-name']))

            self.log.info("Logfile: %s" % "PyUnit_{}.log".format(self.passedScenario.nd_conf_dict['scenario-name']))
            # Store the child PID for later reference.
            child_pid_dict = self.parameters["child_pid"]
            child_pid_dict[self.passedScenario.nd_conf_dict['scenario-name']] = self.childPID
            return True
        else:
            self.log.error("Failed to fork a child process to execute scenario %s." %
                           self.passedScenario.nd_conf_dict['scenario-name'])
            return False

        test_suite = unittest.TestSuite()

        # If the "real" script is "scenario", then we have to reconstruct
        # our scenarios based upon the specified FDS Scenario config.
        if self.passedScenario.nd_conf_dict['real-script'] == "[scenario]":
            # Verify we have a "fds-scenario" option.
            if "fds-scenario" not in self.passedScenario.nd_conf_dict:
                self.log.error("Missing option 'fds-scenario', a path to an FDS Scenario config file, for scenario %s." %
                               self.passedScenario.nd_conf_dict['scenario-name'])
                return False

            self.log.info("Building a test suite based upon FDS Scenario config file %s." %
                           self.passedScenario.nd_conf_dict['fds-scenario'])

            # Reset the scenario section based upon the contents of the
            # specified FDS Scenario config.
            fdscfg = self.parameters["fdscfg"]
            fdscfg.rt_obj.cfg_file = self.passedScenario.nd_conf_dict['fds-scenario']
            del fdscfg.rt_obj.cfg_scenarios[:]
            fdscfg.rt_obj.config_parse()

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
        runner = FDSTestRunner(output=self.passedLogDir, fds_logger=logging.getLogger())
        testResult = runner.run(test_suite)

        return testResult.wasSuccessful()


# This class contains the attributes and methods to
# kill the forked process indicated by the provided scenario name.
class TestKillScenario(TestCase.FDSTestCase):
    def __init__(self, parameters=None, scenario=None):
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_KillScenario,
                                             "Kill scenario",
                                             True)  # Always run.

        self.passedScenario = scenario

    def test_KillScenario(self):
        """
        Test Case:
        Kill the named scenario.
        """

        child_pid_dict = self.parameters["child_pid"]
        killScenarioName = self.passedScenario.nd_conf_dict['kill_scenario']

        if killScenarioName in child_pid_dict:
            self.log.info("Attempting to kill child PID %s from scenario %s." %
                           (child_pid_dict[killScenarioName], killScenarioName))

            fdscfg = self.parameters["fdscfg"]
            localhost = fdscfg.rt_get_obj('cfg_localhost')

            cmd = "kill -9 %s" % child_pid_dict[killScenarioName]
            status = localhost.nd_agent.exec_wait(cmd)

            if status == 0:
                self.log.info("Killed process %s forked for scenario %s." %
                              (child_pid_dict[killScenarioName], killScenarioName))
            else:
                self.log.error("Attempting to kill child PID %s from scenario %s returned status %d." %
                               (child_pid_dict[killScenarioName], killScenarioName, status))
                return False
        else:
            self.log.warning("No child forked for scenario %s." % killScenarioName)
            return False

        # Note: The child process is not removed from the child_pid dictionary.

        return True


# This class contains the attributes and methods to
# join the forked process indicated by the provided scenario name.
class TestJoinScenario(TestCase.FDSTestCase):
    def __init__(self, parameters=None, scenario=None):
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_JoinScenario,
                                             "Join scenario",
                                             True)  # Always run.

        self.passedScenario = scenario

    def test_JoinScenario(self):
        """
        Test Case:
        Join the PID forked from the named scenario.
        """

        result = False
        child_pid_dict = self.parameters["child_pid"]
        joinScenarioName = self.passedScenario.nd_conf_dict['join_scenario']

        if joinScenarioName in child_pid_dict:
            self.log.info("Attempting to join child PID %s from scenario %s." %
                           (child_pid_dict[joinScenarioName], joinScenarioName))

            try:
                pid, status = os.waitpid(child_pid_dict[joinScenarioName], 0)
            except Exception as e:
                self.log.error("Failed to join with child process from scenario {}".format(joinScenarioName))
                self.log.error(e.message)
            else:
                exitStatus, killSignal = divmod(status, 0x100)
                killSignal = hex(killSignal & 0x7F)
                if (pid == child_pid_dict[joinScenarioName]) and (status == 0):
                    self.log.info("Joined process %s forked for scenario %s." %
                                  (child_pid_dict[joinScenarioName], joinScenarioName))
                    result = True
                elif status == 127:
                    self.log.warning("No child process %s found for forked scenario %s." %
                                     (child_pid_dict[joinScenarioName], joinScenarioName))
                    result = True
                else:
                    self.log.error("Attempting to join child PID %s from scenario %s returned status %s = "
                                   "exit status %s and kill signal %s." %
                                   (child_pid_dict[joinScenarioName], joinScenarioName, status, exitStatus, killSignal))

            del child_pid_dict[joinScenarioName]
        else:
            self.log.warning("No child forked for scenario %s." % joinScenarioName)

        return result


# This class contains the attributes and methods to
# log a "marker" in the SysTest log to aid reading.
class TestLogMarker(TestCase.FDSTestCase):
    def __init__(self, parameters=None, scenario=None, marker=None):
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_LogMarker,
                                             "Log marker")

        self.passedScenario = scenario
        self.passedMarker = marker

    def test_LogMarker(self):
        """
        Test Case:
        Log the marker.
        """

        if (self.passedScenario is None) or (self.passedMarker is None):
            self.log.warning("************************************")
            self.log.warning("Log marker called but parameters missing: scenario name or marker text.")
            self.log.warning("************************************")
        else:
            self.log.info("{} {} {}".format('*'*12,self.passedScenario, '*' * 12))
            self.log.info("*** %s" % self.passedMarker)
            self.log.info("{}".format('*' * (12*2 + 2 + len(self.passedScenario))))

        return True


if __name__ == '__main__':
    TestCase.FDSTestCase.fdsGetCmdLineConfigs(sys.argv)
    unittest.main()
