#!/usr/bin/python
#
# Copyright 2014 by Formation Data Systems, Inc.
#

import sys
import os
import getopt
import traceback
import unittest
from unittest.case import (_ExpectedFailure, _UnexpectedSuccess)
import functools
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
pyUnitTCFailure = False

# This global is ued to indicate whether we've already
# set up the logging facilities during a PyUnit run.
log = None

# This global is ued to indicate whether we've already
# generated parameters for the test fixture.
_parameters = None


def expectedFailure(problem=None):
    def expectedFailure_decorator(func):
        @functools.wraps(func)
        def wrapper(*args, **kwargs):
            if problem is not None:
                log.info("Expected failure due to %s." % problem)

            try:
                return func(*args, **kwargs)
            except Exception as e:
                raise _ExpectedFailure(sys.exc_info())
        wrapper.__expected_failure__ = True
        return wrapper
    return expectedFailure_decorator


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

        print "pyUnitConfig: ", pyUnitConfig
        print "pyUnitVerbose: ", pyUnitVerbose
        print "pyUnitDrynrun: ", pyUnitDryrun
        print "pyUnitInstall: ", pyUnitInstall
        print "pyUnitSudoPw: ", pyUnitSudoPw
        print "pyUnitInventory", pyUnitInventory

        _parameters = TestUtils.get_config(True, pyUnitConfig, pyUnitVerbose, pyUnitDryrun, pyUnitInstall, pyUnitSudoPw, pyUnitInventory)

        # Set up logging. We wait to do it here after we've parsed
        # the qaautotest.ini file (also used for PyUnit runs)
        # which identifies the log directory. But we only want to
        # do it once per test run, hence the global "log" variable.
        if log is None:
            log = TestUtils._setup_logging("PyUnit.log",
                                           _parameters["log_dir"], _parameters["log_level"])

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
            log.info("FDS config file (.ini config 'fds_config_file'): %s." % _parameters["fds_config_file"])


class FDSTestCase(unittest.TestCase):

    def __init__(self,
                 parameters=None,
                 testCaseName=None,
                 testCaseDriver=None,
                 testCaseDescription=None,
                 testCaseAlwaysExecute=False,
                 fork=False):
        """
        When run by a qaautotest module test runner,
        this method provides the test fixture allocation.

        When run by the PyUnit test runner, this method
        sets the config file specified on the command line.

        To consolidate logic, we'll let method setUp() handle it.
        """
        super(FDSTestCase, self).__init__()

        self.setUp(parameters)

        self.passedTestCaseName = testCaseName
        self.passedTestCaseDriver = testCaseDriver
        self.passedTestCaseDescription = testCaseDescription
        self.passTestCaseAlwaysExecute = testCaseAlwaysExecute
        self.passedFork = fork

        self.childPID = None

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

    def checkS3Info(self, bucket=None):
        if (not "s3" in self.parameters) or (self.parameters["s3"].conn) is None:
            self.log.error("No S3 connection")
            return False

        if bucket is not None:
            self.parameters["s3"].bucket1 = self.parameters["s3"].conn.lookup(bucket)

        if not self.parameters["s3"].bucket1:
            self.log.error("No S3 bucket info")
            return False

        return True

    def runTest(self):
        """
        Define this one in your specific test case class to run or
        invoke the test case execution and then do any necessary
        test fixture cleanup.

        For quautotest, return "True" if the test case passed, "False" otherwise.
        For PyUnit, use a unittest.assert* method to assess test case results.
        """
        global pyUnitTCFailure

        test_passed = True
        printHeader=True
        if (self.__class__.__name__ == "TestLogMarker") or (self.__class__.__name__ == "TestWait"):
            printHeader = False

        if pyUnitTCFailure and not self.passTestCaseAlwaysExecute:
            self.log.warning("Skipping Case %s. stop-on-fail/failfast set and a previous test case has failed." %
                             self.__class__.__name__)
            return unittest.skip("stop-on-fail/failfast set and a previous test case has failed.")
        else:
            if printHeader:
                self.log.info("Running Case %s." % self.__class__.__name__)

        try:
            if not self.passedTestCaseDriver():
                test_passed = False
        except _ExpectedFailure as e:
            test_passed = False
        except Exception as e:
            self.log.error("%s caused exception:" % self.passedTestCaseDescription)
            self.log.error(traceback.format_exc())
            self.log.error(e.message)
            test_passed = False

        self.reportTestCaseResult(test_passed, hasattr(self.passedTestCaseDriver, "__expected_failure__"))

        # If there is any test fixture teardown to be done, do it here.

        # Were we forking?
        if self.childPID is not None:
            # Yes we were.
            # If this is the child, we'll
            # exit here. The parent falls
            # through.
            if self.childPID == 0:
                os._exit(0 if test_passed else -1)

        if self.parameters["pyUnit"]:
            # If the test case driver method has been decorated with "expected failure",
            # then we have either an unexpected success
            # or an expected failure. We will know which by whether the test
            # case passed or not. In these cases we will want to raise the
            # appropriate exception to the parent class in unittest so that
            # its results can be recorded accordingly.
            if not hasattr(self.passedTestCaseDriver, "__expected_failure__"):
                self.assertTrue(test_passed)
            elif test_passed:
                raise _UnexpectedSuccess
            else:
                raise _ExpectedFailure(sys.exc_info())
        else:
            return test_passed

    def reportTestCaseResult(self, test_passed, failExpected):
        """
        Just a quick report of results for the test case.
        """
        global pyUnitTCFailure

        printFooter=True
        if (self.__class__.__name__ == "TestLogMarker") or (self.__class__.__name__ == "TestWait"):
            printFooter = False

        if test_passed:
            if failExpected:
                self.log.warn("Test Case %s passed unexpectedly." % self.__class__.__name__)
            else:
                if printFooter:
                    self.log.info("Test Case %s passed." % self.__class__.__name__)
        else:
            if failExpected:
                if printFooter:
                    self.log.info("Test Case %s failed as expected." % self.__class__.__name__)
            else:
                self.log.error("Test Case %s failed." % self.__class__.__name__)

                if self.parameters["stop_on_fail"]:
                    pyUnitTCFailure = True


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
        global pyUnitInventory

        log_dir = None
        failfast = False

        # We must have the -i option but that will be checked later.
        options, args = getopt.getopt(_argv[1:], 'q:l:d:z:vrif', ['qat-file', 'log-dir', 'sudo-password', 'inventory-file', 'verbose',
                                                                'dryrun', 'install', 'failfast'])

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
            elif opt in ('-f','--failfast'):
                failfast = True
                _argv.pop(idx)
            elif opt in ('-z', '--inventory-file'):
                pyUnitInventory = value
                _argv.pop(idx)
                _argv.pop(idx)
            else:
                idx += 1

        # Instantiate an FDSTestCase so we can initialize our fixture from the configuration file.
        initialCase = FDSTestCase(parameters=None)

        # From either the command line or from the config file, one must
        # have given us a log_dir.
        if log_dir is None:
            log_dir = _parameters["log_dir"]

        # Make sure we're covered with the stop-on-fail/failfast option.
        # qaautotest calls it stop-on-fail. pyUnit calls it failfast
        if _parameters["stop_on_fail"]:
            failfast = True

        if failfast:
            _parameters["stop_on_fail"] = True

        #print "Parameters: ", _parameters
        return log_dir, failfast, pyUnitInstall
