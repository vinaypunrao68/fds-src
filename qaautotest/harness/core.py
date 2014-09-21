#!/usr/bin/env python

import os
import sys
import logging

from optparse import OptionParser
if sys.version_info[0] < 3:
    import ConfigParser as configparser
else:
    import configparser


class SuiteDirectives(object):
    def __init__(self, config):
        self.config = config
        self.log = logging.getLogger("SuiteDirectives")
        
    def start(self):
        self.log.debug("Running pre-suite validation.")
        return True
        
    def end(self):
        self.log.debug("Running post-suite validation.")
        return True
    
    
class TestCaseDirectives(object):
    def __init__(self, test_path, config):
        self.test_path = test_path
        self.config = config
        self.log = logging.getLogger("TestDirectives")
        
    def start(self):
        success = True
        self.log.debug("Running pre-test case setup.")
        if not self.__check_uid():
            success = False
        return success
    
    def end(self):
        self.log.debug("Running post test case cleanup.")
        return True
    
    def __check_uid(self):
        if run_as_root or self.config["run_as_root"]:
            if os.geteuid() != 0:
                self.log.error("Test case is configured to run as root but "
                               "you are not root.  Running as UID %s."
                               %os.geteuid())
                return False
        return True
    
def get_options():
    parser = OptionParser()
    parser.prog = sys.argv[0].split("/")[-1]
    parser.usage = "%prog -h | <Suite|.csv|File:Class> \n" + \
                   "<-c <config.ini> | -s <test_source_dir>> " + \
                   "[-b <build_num>] \n[-l <log_dir>] [--level <log_level>]" + \
                   "[--stop-on-fail] [--run-as-root] \n" + \
                   "[--iterations <num_iterations>] [--store]>"
    
    parser.add_option("-c", "--config", action="store", type="string",
                      dest="config", help="The file containing the "
                      "configuration information needed to run the tests.")
    parser.add_option("-s", "--src-dir", action="store", type="string",
                      dest="test_source_dir", help="The path to the directory "
                      "in which the tests may be found.")
    parser.add_option("-b", "--build", action="store", type="string",
                      dest="build", default="Unspecified", help="The build "
                      "number of the code under test.")
    parser.add_option("-l", "--log-dir", action="store", type="string",
                      dest="log_dir", default=".", help="The directory in "
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
    
    validate_cli_options(parser)
    return parser
    
def validate_cli_options(parser):
    (options, args) = parser.parse_args()
    if len(args) == 0:
        parser.error("The first argument must be the test name, the suite "
                     "name, or .csv file in which a list of tests can be found.")
    if len(args) > 1:
        parser.error("Multiple positional arguments have been specified.")
    if options.store_to_database == True and options.build == "Unspecified":
        parser.error("To store test results to the database, the build number "
                     "must be specified.  If you do not have a build number, "
                     "enter the date, e.g. 2010-09-17.")
    try:
        if int(options.iterations) == -1:
            options.iterations = pow(2, 24) # infinite enough
    except (ValueError, TypeError):
        pass
    
def get_config():
    """ Configuration can be gathered from one of two sources: 1) a
    configuration .ini file and/or 2) the command line.  Configuration settings
    will first be imported from the file, if the option has been specified.
    Then, command line options will be applied.  When specified in both places,
    command line arguments will override those defined in the configuration
    file.
    
    Following the import of the configuration, the settngs will be scrubbed.
    Strings will be converted to int or bool where appropriate.  Paths on
    the file system will be resolved to their absolute paths.
    
    Note that if the file system is already mounted, the multicast IP, export,
    and mount point will be taken from the already mounted file system,
    irrespective of what has been specified in the configuration .ini file or
    the command line.
    """
    # Initialize variables.
    # Import all options passed in at the command line.
    params = {}
    parser = get_options()
    (options, args) = parser.parse_args()
    try:
        params["tests"] = args[0]
    except IndexError:
        print("The first argument must be the test name, the suite name, "
              "or .csv file in which a list of tests can be found.")
        sys.exit(1)
    
    # Process configuration parameters specified in the configuration .ini
    # file.
    if options.config != None:
        if not os.path.exists(options.config):
            print("The configuration .ini file specified doesn't exist: %s"
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
                params[key] = os.path.abspath(value)
                
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
    
    # The test directory, i.e. the place where the test
    # code may be found, must be specified.
    if (params["test_source_dir"] == None):
        print("The test directory is not specified.  Add the -s option to "
              "your command line or add the mount item to the configuration "
              ".ini file.")
        sys.exit(1)
    
    global run_as_root
    if params["run_as_root"] == True:
        run_as_root = True
    else:
        run_as_root = False
    
    return params
    
