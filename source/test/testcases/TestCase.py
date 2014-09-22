#!/usr/bin/python
#
# Copyright 2014 by Formation Data Systems, Inc.
#

import sys
import getopt
import traceback
import unittest
import logging
import fdslib.FdsLogging
import core as profile

# This global is used to pass the path and name of the config
# file to use with a PyUnit run. It is the same .ini file as would be
# used with a qaautotest run.
pyUnitConfig = None

# These globals are used to pass execution switches when run
# using the PyUnit test runner.
pyUnitVerbose = False
pyUnitDryrun = False
pyUnitInstall = False

# This global is ued to indicate whether we've already
# set up the logging facilities during a PyUnit run.
log = None

class FDSTestCase(unittest.TestCase):

    def __init__(self, parameters=None):
        """
        When run by a qaautotest module test runner,
        this method provides the test fixture allocation.

        When run by the PyUnit test runner, this method
        sets the config file specified on the command line.

        To consolidate logic, we'll let method setUp() handle it.
        """
        super(FDSTestCase, self).__init__()

        # TODO(Greg): PyUnit seems to call this method at unexpected times.
        #if not isinstance(parameters, dict):
        #    parameters = None

        self.setUp(parameters)


    def setUp(self, parameters=None):
        """
        When run by the PyUnit test runner,
        this method provides the test fixture allocation.
        In this case, parameters would be "None". Otherwise,
        when being run by the qaautotest module test runner,
        parameters will be constructed and passed to us from
        the test harness.
        """
        if parameters is None:
            global pyUnitConfig
            global log

            # We assume that we are being called via the PyUnit test runner.
            # But we must have pyUnitConfig from the command line.
            if pyUnitConfig == None:
                # Let's see if we can tease it out of sys.argv.
                qaautotestini = sys.argv.index('-q')
                if qaautotestini == 0:
                    print("No qaautotest.ini specified. This would be command line option '-q'.")
                    traceback.print_stack()
                    sys.exit(1)
                else:
                    pyUnitConfig = sys.argv[qaautotestini+1]
                    print("Teased out qaautotest.ini as %s." % pyUnitConfig)

            self.parameters = profile.get_config(True, pyUnitConfig, pyUnitVerbose, pyUnitDryrun, pyUnitInstall)

            # Set up logging. We wait to do it here after we've parsed
            # the qaautotest.ini file (also used for PyUnit runs)
            # which identifies the log directory. But we only want to
            # do it once per test run, hence the global "log" variable.
            if log is None:
                log = fdslib.FdsLogging._setup_logging.__func__(self.__class__.__name__, "PyUnit.log",
                                                                self.parameters["log_dir"], self.parameters["log_level"],
                                                                self.parameters["threads"])
        else:
            self.parameters = parameters  # Passed in from the harness

        self.log = logging.getLogger(self.__class__.__name__)

        # TODO(Greg): PyUnit seems to call this method at unexpected times.
        ## Log a few useful things to know about this run.
        #self.log.info("QAAutoTest harness .ini (-q|--qat-file): %s." % parameters["config"])
        #self.log.info("Source of QAAutoTest harness test cases (-s|--src_dir or .ini config 'test_source_dir'): %s." %
        #              parameters["test_source_dir"])
        #self.log.info("Build under test (-b|--build or .ini config 'build'): %s." % parameters["build"])
        #self.log.info("Test log directory (-l|--log-dir or .ini config 'log_dir'): %s." % parameters["log_dir"])
        #self.log.info("Test log level (--level or .ini config 'log_level'): %d." % parameters["log_level"])
        #self.log.info("Test threads (--threads or .ini config 'threads'): %d." % parameters["threads"])
        #self.log.info("Test iterations (---iterations or .ini config 'iterations'): %d." % parameters["iterations"])
        #self.log.info("Stop on fail (--stop-on-fail or .ini config 'stop_on_fail'): %s." % parameters["stop_on_fail"])
        #self.log.info("Run as root (--run-as-root or .ini config 'run_as_root'): %s." % parameters["run_as_root"])
        #self.log.info("Verbose logging (-v|--verbose or .ini config 'verbose'): %s." % parameters["verbose"])
        #self.log.info("'Dry run' test (-r|--dryrun or .ini config 'dryrun'): %s." % parameters["dryrun"])
        #self.log.info("Install from release package (-i|--install or .ini config 'install'): %s." % parameters["install"])
        #self.log.info("FDS config file (.ini config 'fds_config_file'): %s." % parameters["fds_config_file"])


    def tearDown(self):
        """
        When run by the PyUnit test runner,
        this method provides the test fixture release.

        In the case of qaautotest, it appears you must do
        this yourself, preferably as part of the test case
        execution - the runTest() method.
        """
        pass

    def runTest(self):
        """
        Define this one in your specific test case class to run or
        invoke the test case execution and then do any necessary
        test fixture cleanup.

        For quautotest, return "True" if the test case passed, "False" otherwise.
        For PyUnit, use a unittest.assert* method to assess test case results.
        """
        test_passed = True

        FDSTestCase.reportTestCaseResult(self, test_passed)

        if self.parameters["pyUnit"]:
            self.assertTrue(test_passed)
        else:
            return test_passed

    def reportTestCaseResult(self, test_passed):
        """
        Just a quick report of results for the test case.
        """
        if test_passed:
            self.log.info("Test Case %s passed." % self.__class__.__name__)
        else:
            self.log.info("Test Case %s failed." % self.__class__.__name__)


    @staticmethod
    def fdsGetCmdLineConfigs(_argv):
        """
        Called before unittest.main() when executed by the
        PyUnit test runner.

        Retrieve any command line arguments/options that are
        specific to FDS as opposed to PyUnit.
        """

        global pyUnitConfig
        global pyUnitVerbose
        global pyUnitDryrun
        global pyUnitInstall

        # We must have the -i option but that will be checked later.
        options, args = getopt.getopt(_argv[1:], 'q:vri', ['qat_file', 'verbose', 'dryrun' 'install'])

        idx = 1
        for opt, value in options:
            if opt in ('-q','--qat_file'):
                pyUnitConfig = value
                # Remove this option and its argument from argv so as not to confuse PyUnit.
                _argv.pop(idx)
                _argv.pop(idx)
            elif opt in ('-v','--verbose'):
                pyUnitVerbose = True
                # Remove this option from argv so as not to confuse PyUnit.
                #_argv.pop(idx)  Actually, PyUnit has a --verbose option, so maybe we don't want to remove it.
            elif opt in ('-r','--dryrun'):
                pyUnitDryrun = True
                # Remove this option from argv so as not to confuse PyUnit.
                _argv.pop(idx)
            elif opt in ('-i','--install'):
                pyUnitInstall = True
                # Remove this option from argv so as not to confuse PyUnit.
                _argv.pop(idx)
            else:
                idx += 1

