#!/usr/bin/python
#
# Copyright 2014 by Formation Data Systems, Inc.
#
import os
import sys
import logging
import logging.handlers

from optparse import OptionParser
if sys.version_info[0] < 3:
    import ConfigParser as configparser
else:
    import configparser

import BringUpCfg as fdscfg

def _setup_logging(logger_name, log_name, dir, log_level, num_threads, max_bytes=100*1024*1024, rollover_count=5):
    # Set up the core logging engine
    #logging.basicConfig(level=log_level,
    #            format='%(asctime)s (%(thread)d) %(name)-24s %(levelname)-8s %(message)s',
    #            datefmt='%Y-%m-%d %H:%M:%S')
    log = logging.getLogger(logger_name)
    log.setLevel(log_level)
    logging.addLevelName(100, "REPORT")

    # Set up the log file and log file rotation
    if not os.path.isdir(dir):
        os.makedirs(dir)

    log_file = os.path.join(dir, log_name)
    print("Logfile: %s"
              %log_file)
    if num_threads > 1:
        log_fmt = logging.Formatter("%(asctime)s (%(thread)d) %(name)-24s %(levelname)-8s %(message)s",
                                    "%Y-%m-%d %H:%M:%S")
    else:
        log_fmt = logging.Formatter("%(asctime)s %(name)-24s %(levelname)-8s %(message)s",
                                    "%Y-%m-%d %H:%M:%S")
    if not os.access(dir, os.W_OK):
       log.warning("There is no write access to the specified log "
                         "directory %s  No log file will be created." %dir)
    else:
        try:
            log_handle = logging.handlers.RotatingFileHandler(log_file, "w",
                                                              max_bytes,
                                                              rollover_count)
            log_handle.setFormatter(log_fmt)
            logging.getLogger("").addHandler(log_handle)
            if os.path.exists(log_file):
                log_handle.doRollover()
        except:
            log.warning("Failed to rollover the log file.  The "
                             "results will not be recorded.")
            logging.getLogger("").removeHandler(log_handle)
    #log.log(100, " ".join(sys.argv))

    return log

def get_options(pyUnit):
    parser = OptionParser()
    parser.prog = sys.argv[0].split("/")[-1]
    parser.usage = "%prog -h | <Suite|.csv|File:Class> \n" + \
                   "<-q <qaautotest.ini> | -s <test_source_dir>> " + \
                   "[-b <build_num>] \n[-l <log_dir>] [--level <log_level>]" + \
                   "[--stop-on-fail] [--run-as-root] \n" + \
                   "[--iterations <num_iterations>] [--store] \n" + \
                   "[-v|--verbose] [-r|--dryrun]> [-i|--install]>"

    # FDS: Changed to option i/ini_file from c/config to prevent clashing with PyUnit's c/catch option.
    parser.add_option("-q", "--qat-file", action="store", type="string",
                      dest="config", help="The file containing the test harness "
                      "configuration information. (qaautotest's .ini file.)")
    parser.add_option("-s", "--src-dir", action="store", type="string",
                      dest="test_source_dir", help="The path to the directory "
                      "in which the tests may be found.")
    parser.add_option("-b", "--build", action="store", type="string",
                      dest="build", default="Unspecified", help="The build "
                      "number of the code under test.")
    #FDS: Option -l used to have default='.' but this overrode configuration .ini settings.
    parser.add_option("-l", "--log-dir", action="store", type="string",
                      dest="log_dir", help="The directory in "
                      "which the log file should be located.  If unspecified, "
                      "the current directory will be used.")
    parser.add_option("--level", action="store", type="string",
                      dest="log_level", help="The log level for the harness "
                      "logger, e.g. logging.DEBUG, 20, etc.")
    parser.add_option("--threads", action="store", type="int",
                      default=1, dest="threads", help="If unset, the tests "
                      "will be run serially, i.e. in a single-threaded mode.  "
                      "To run tests in parallel, set the number of threads to "
                      "use.  Each test case will be run in its own thread.")
    parser.add_option("--iterations", action="store", type="int",
                      dest="iterations", help="If unset, the specified tests "
                      "will be run once.  Otherwise, the tests are repeated "
                      "the specified number of iterations.  Enter -1 for "
                      "infinite.")
    parser.add_option("--stop-on-fail", action="store_true",
                      dest="stop_on_fail", help="If set, the harness will stop "
                      "immediately if a test run fails.")
    parser.add_option("--run-as-root", action="store_true",
                      dest="run_as_root", help="If set, the test cases will "
                      "all be run as the root user.")
    parser.add_option("--store", action="store_true",
                      dest="store_to_database", help="If set, the results of "
                      "the test run will be archived in the test database.")
    # FDS: Add a couple of FDS-specific options.
    parser.add_option('-v', '--verbose', action = 'store_true', dest = 'verbose',
                      default = False, help = 'enable verbosity')
    parser.add_option('-r', '--dryrun', action = 'store_true', dest = 'dryrun',
                      default = False, help = 'dry run, print commands only')
    parser.add_option('-i', '--install', action = 'store_true', dest = 'tar_file',
                      default = False, help = 'perform an install from an FDS package as opposed '
                                              'to a development environment')
    parser.add_option('-d', '--sudo-password', action = 'store', dest = 'sudo_password',
                      help = 'When the node is localhost, use this password for sudo access to the configured user. ')

    validate_cli_options(parser, pyUnit)
    return parser

def validate_cli_options(parser, pyUnit):
    (options, args) = parser.parse_args()
    if (len(args) == 0) and (pyUnit == False):
        parser.error("The first argument must be the test name, the suite "
                     "name, or .csv file in which a list of tests can be found.")
    if len(args) > 1:
        parser.error("Multiple positional arguments have been specified.")
    if (options.store_to_database == True and options.build == "Unspecified") and (pyUnit == False):
        parser.error("To store test results to the database, the build number "
                     "must be specified.  If you do not have a build number, "
                     "enter the date, e.g. 2010-09-17.")
    try:
        if int(options.iterations) == -1:
            options.iterations = pow(2, 24) # infinite enough
    except (ValueError, TypeError):
        pass

def get_config(pyUnit = False, pyUnitConfig = None, pyUnitVerbose = False, pyUnitDryrun = False, pyUnitInstall = False, pyUnitSudoPw = None):
    """ Configuration can be gathered from one of two sources: 1) a
    configuration .ini file and/or 2) the command line.  Configuration settings
    will first be imported from the file, if the option has been specified.
    Then, command line options will be applied.  When specified in both places,
    command line arguments will override those defined in the configuration
    file. FDS: So be careful about what options have defaults. In those cases, the
    value in the configuration.ini will be ignored even if *not* specified
    on the command line.

    Following the import of the configuration, the settngs will be scrubbed.
    Strings will be converted to int or bool where appropriate.  Paths on
    the file system will be resolved to their absolute paths.

    Note that if the file system is already mounted, the multicast IP, export,
    and mount point will be taken from the already mounted file system,
    irrespective of what has been specified in the configuration .ini file or
    the command line.

    FDS: With pyUnit = True, we understand that we are being called in support
    of a test being run with Python's unittest module infrastructure.
    """

    # In the event we try to log messages before we've set up logging...
    logging.basicConfig(level=logging.ERROR,
                format='%(asctime)s (%(thread)d) %(name)-24s %(levelname)-8s %(message)s',
                datefmt='%Y-%m-%d %H:%M:%S')

    # Initialize variables.
    # Import all options passed in at the command line.
    params = {}

    # FDS: We must have this config file specified.
    params["fds_config_file"] = None

    parser = get_options(pyUnit)
    (options, args) = parser.parse_args()
    if pyUnit == False:
        try:
            params["tests"] = args[0]
        except IndexError:
            print("The first argument must be the test name, the suite name, "
                  "or .csv file in which a list of tests can be found.")
            sys.exit(1)
    else:
        # When run under PyUnit, we expect the config file to be passed
        # in as pyUnitConfig. But it should be the same as the .ini
        # file used by a qaautotest run.
        options.config = pyUnitConfig

    # Process configuration parameters specified in the configuration .ini
    # file.
    if options.config != None:
        if not os.path.exists(options.config):
            print("The qaautotest configuration .ini file specified doesn't exist: %s"
                  %options.config)
            sys.exit(1)
        config = configparser.SafeConfigParser()
        try:
            config.read(options.config)
        except configparser.MissingSectionHeaderError:
            print("The configuration file %s is not in the expected "
                  "format.  It contains no section headers."
                  %options.config)
            sys.exit(1)
        try:
            print("Importing configuration settings from file: %s"
                  %options.config)
            for item in config.items("harness"):
                params[item[0]] = item[1].split()[0]
        except configparser.NoSectionError:
            print("The configuration file %s has no section \"[cluster]\".  "
                  "This section is required." %options.config)
            sys.exit(1)

    # Process parameters defined at the command line.  These will override
    # anything set in the .ini file.
    for key, value in options.__dict__.items():
        if key in params and params[key] == None:
            params[key] = value
        elif key in params and params[key] == "":
            params[key] = value
        elif key in params and value != None:
            params[key] = value
        if key not in params:
            params[key] = value

    # Apply some sanity conversions.  str->int and str->bool
    # Also, make all path parameters absolute paths
    for key, value in params.items():
        try:
            if isinstance(params[key], str):
                params[key] = int(value)
        except (ValueError, TypeError):
            pass
        if value == "True":
            params[key] = True
        elif value == "False":
            params[key] = False
        if isinstance(params[key], str):
            if (key == "test_source_dir") or (key == "log_dir") or (key == "config"):
                if params[key][0] == '~':
                    params[key] = os.path.expanduser(params[key])
                elif params[key][0] != '/':
                    params[key] = os.path.abspath(params[key])

                #params[key] = os.path.abspath(value)

    # Ensure the number of iterations is valid
    try:
        if not isinstance(params["iterations"], int):
            params["iterations"] = 1
    except KeyError:
        params["iterations"] = 1

    # Check also the logging option to make sure it's in integer format:
    level_options = {"critical": 50,
                     "error": 40,
                     "warning": 30,
                     "info": 20,
                     "debug": 10,
                     "notset": 0}
    if not isinstance(params["log_level"], int):
        try:
            if "logging" in params["log_level"]:
                p = params["log_level"].split(".")[1].lower()
            else:
                p = params["log_level"].lower()
            params["log_level"] = level_options[p]
        except (TypeError, KeyError):
            params["log_level"] = logging.DEBUG

    logging.getLogger().setLevel(params["log_level"])

    # The test directory, i.e. the place where the test
    # code may be found, must be specified.
    if not pyUnit:
        if params["test_source_dir"] is None:
            print("The test directory is not specified.  Add the -s option to "
                  "your command line or add the mount item to the configuration "
                  ".ini file.")
            sys.exit(1)
        elif not os.path.isdir(params["test_source_dir"]):
            print("The test directory specified, test_source-dir=%s, does not exist." % params["test_source_dir"])
            sys.exit(1)


    # FDS: The log directory, i.e. the place where the test
    # logs may be written, must be specified.
    if params["log_dir"] is None:
        if pyUnit:
            print("The log directory is not specified.  Add the log_dir item to "
                  "your config file: %s." % options.config)
        else:
            print("The log directory is not specified.  Add the -l option to "
                  "your command line or add the log_dir item to the configuration "
                  ".ini file.")
        sys.exit(1)

    # FDS: Check the passed options in case we are running from PyUnit
    if params["verbose"] == False:
        params["verbose"] = pyUnitVerbose
        setattr(options, "verbose", params["verbose"])
    if params["dryrun"] == False:
        params["dryrun"] = pyUnitDryrun
        setattr(options, "dryrun", params["dryrun"])
    if params["tar_file"] == False:
        params["tar_file"] = pyUnitInstall
        setattr(options, "tar_file", params["tar_file"])
    if params["sudo_password"] is None:
        params["sudo_password"] = pyUnitSudoPw
        setattr(options, "sudo_password", params["sudo_password"])

    global run_as_root
    if params["run_as_root"] == True:
        run_as_root = True
    else:
        run_as_root = False

    # FDS: We must have an FDS config file specified in the qaautotest .ini file for the suite.
    if params["fds_config_file"] is None:
        print("You need to pass item fds_config_file in your qaautotest test suite .ini file: %s"
                  %options.config)
        sys.exit(1)
    setattr(options, "config_file", params["fds_config_file"])

    if not os.path.isfile(options.config_file):
        print("The fds_config_file item, %s, in your qaautotest test suite .ini file does not exist." % options.config)
        sys.exit(1)

    # FDS: This one is not required, particularly if we want to run from
    # an installation package.
    if params["fds_source_dir"] is None:
        params["fds_source_dir"] = ''
    setattr(options, "fds_source_dir", params["fds_source_dir"])

    # FDS: Some FDS root directory must be specified to satisfy dependencies in FdsConfigRun.
    # But it will be correctly set when the FDS config file is parsed for each identified node.
    #
    # The first parameter to FdsConfigRun is an FdsEnv which will be determined
    # once we have the FDS root directory right.
    setattr(options, "fds_root", '.')
    params["fdscfg"] = fdscfg.FdsConfigRun(None, options, test_harness=True)

    # FDS: Now record whether we are being run by PyUnit.
    params["pyUnit"] = pyUnit

    return params

