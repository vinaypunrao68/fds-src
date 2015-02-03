#!/usr/bin/python
# coding: utf8

import argparse
import getopt
import sys

import config
import fdslib.restendpoint as rep
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
xargs = None
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

        print "pyUnitConfig: ", pyUnitConfig
        print "pyUnitVerbose: ", pyUnitVerbose
        print "pyUnitDrynrun: ", pyUnitDryrun
        print "pyUnitInstall: ", pyUnitInstall
        print "pyUnitSudoPw: ", pyUnitSudoPw
        _parameters = TestUtils.get_config(True, pyUnitConfig, pyUnitVerbose, pyUnitDryrun, pyUnitInstall, pyUnitSudoPw)
        print _parameters
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
            log.info("Install from release package (-i|--install or .ini config 'install'): %s." % _parameters["tar_file"])
            log.info("FDS config file (.ini config 'fds_config_file'): %s." % _parameters["fds_config_file"])


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
    failfast = False
    fdscfg = None
    # We must have the -i option but that will be checked later.
    options, args = getopt.getopt(_argv[1:], 'q:l:d:vrif', ['qat-file', 'log-dir', 'sudo-password', 'verbose',
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
        else:
            idx += 1

    # Instantiate an FDSTestCase so we can initialize our fixture from the configuration file.
    if _parameters == None:
        setUpModule()
        fdscfg =  _parameters

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

    return fdscfg

if __name__ == "__main__":
    fdsGetCmdLineConfigs(config.CMD_CONFIG)
