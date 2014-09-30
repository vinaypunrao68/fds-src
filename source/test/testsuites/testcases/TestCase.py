#!/usr/bin/python
#
# Copyright 2014 by Formation Data Systems, Inc.
#

import sys
import getopt
import traceback
import unittest
import logging
import fdslib.TestUtils as TestUtils

# This global is used to pass the path and name of the config
# file to use with a PyUnit run. It is the same .ini file as would be
# used with a qaautotest run.
pyUnitConfig = None

# These globals are used to pass execution switches when run
# using the PyUnit test runner.
pyUnitVerbose = False
pyUnitDryrun = False
pyUnitInstall = False
pyUnitSudoPw = None

# This global is ued to indicate whether we've already
# set up the logging facilities during a PyUnit run.
log = None

# This global is ued to indicate whether we've already
# generated parameters for the test fixture.
_parameters = None

def setUpModule():
    """
    When run by the PyUnit test runner,
    this method provides the test fixture allocation.
    In this case, _parameters would be "None". Otherwise,
    when being run by the qaautotest module test runner,
    parameters will be constructed and passed to us from
    the test harness.
    """
    global _parameters

    if _parameters is None:
        global pyUnitConfig
        global log

        # We assume that we are being called via the PyUnit test runner.
        # But we must have pyUnitConfig from the command line.
        if pyUnitConfig is None:
            # Let's see if we can tease it out of sys.argv.
            qaautotestini = sys.argv.index('-q')
            if qaautotestini == 0:
                print("No qaautotest.ini specified. This would be command line option '-q'.")
                traceback.print_stack()
                sys.exit(1)
            else:
                pyUnitConfig = sys.argv[qaautotestini+1]
                print("Teased out qaautotest.ini as %s." % pyUnitConfig)

        _parameters = TestUtils.get_config(True, pyUnitConfig, pyUnitVerbose, pyUnitDryrun, pyUnitInstall, pyUnitSudoPw)

        # Set up logging. We wait to do it here after we've parsed
        # the qaautotest.ini file (also used for PyUnit runs)
        # which identifies the log directory. But we only want to
        # do it once per test run, hence the global "log" variable.
        if log is None:
            log = TestUtils._setup_logging(__name__, "PyUnit.log",
                                           _parameters["log_dir"], _parameters["log_level"],
                                           _parameters["threads"])

            # Log a few useful things to know about this run.
            log.info("QAAutoTest harness .ini (-q|--qat-file): %s." % _parameters["config"])
            log.info("Source of QAAutoTest harness test cases (-s|--src_dir or .ini config 'test_source_dir'): %s." %
                    _parameters["test_source_dir"])
            log.info("Build under test (-b|--build or .ini config 'build'): %s." % _parameters["build"])
            log.info("Test log directory (-l|--log-dir or .ini config 'log_dir'): %s." % _parameters["log_dir"])
            log.info("Test log level (--level or .ini config 'log_level'): %d." % _parameters["log_level"])
            log.info("Test threads (--threads or .ini config 'threads'): %d." % _parameters["threads"])
            log.info("Test iterations (---iterations or .ini config 'iterations'): %d." % _parameters["iterations"])
            log.info("Stop on fail (--stop-on-fail or .ini config 'stop_on_fail'): %s." % _parameters["stop_on_fail"])
            log.info("Run as root (--run-as-root or .ini config 'run_as_root'): %s." % _parameters["run_as_root"])
            log.info("Verbose logging (-v|--verbose or .ini config 'verbose'): %s." % _parameters["verbose"])
            log.info("'Dry run' test (-r|--dryrun or .ini config 'dryrun'): %s." % _parameters["dryrun"])
            log.info("Install from release package (-i|--install or .ini config 'install'): %s." % _parameters["install"])
            log.info("FDS config file (.ini config 'fds_config_file'): %s." % _parameters["fds_config_file"])


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
            setUpModule()
            self.parameters = _parameters
        else:
            self.parameters = parameters  # Passed in from the harness

        self.log = logging.getLogger(self.__class__.__name__)


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
        global pyUnitSudoPw

        log_dir = None

        # We must have the -i option but that will be checked later.
        options, args = getopt.getopt(_argv[1:], 'q:l:d:vri', ['qat-file', 'log-dir', 'sudo-password', 'verbose', 'dryrun' 'install'])

        idx = 1
        for opt, value in options:
            if opt in ('-q','--qat-file'):
                pyUnitConfig = value
                # Remove this option and its argument from argv so as not to confuse PyUnit.
                _argv.pop(idx)
                _argv.pop(idx)
            elif opt in ('-v','--verbose'):
                pyUnitVerbose = True
                # PyUnit has a --verbose option, so we don't want to remove it.
            elif opt in ('-r','--dryrun'):
                pyUnitDryrun = True
                # Remove this option from argv so as not to confuse PyUnit.
                _argv.pop(idx)
            elif opt in ('-i','--install'):
                pyUnitInstall = True
                # Remove this option from argv so as not to confuse PyUnit.
                _argv.pop(idx)
            elif opt in ('-l','--log-dir'):
                log_dir = value
                # Leave this argument to be used by subsequent config setup.
            elif opt in ('-d','--sudo-password'):
                pyUnitSudoPw = value
                # Remove this option and its argument from argv so as not to confuse PyUnit.
                _argv.pop(idx)
                _argv.pop(idx)
            else:
                idx += 1

        return log_dir