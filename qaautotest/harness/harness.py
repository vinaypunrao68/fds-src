#!/usr/bin/env python

import sys, os, re, time
import copy
import signal
import traceback
import threading
import logging
import logging.handlers
if sys.version_info[0] < 3:
    import ConfigParser as configparser
else:
    import configparser

import core as profile

try:
    import MySQLdb
    from dbaccess import dbaccess
except ImportError:
    pass


def main():
    config = profile.get_config()
    harness = Harness(config["build"], config["log_dir"], config["log_level"], config["threads"])
    harness.initialize_database(os.path.join(os.path.split(os.path.abspath(__file__))[0], "database.ini"))
    harness.add_tests(config["test_source_dir"], config["tests"])
    harness.run_tests(config, config["store_to_database"], config["stop_on_fail"], config["iterations"], config["threads"])
    failures = harness.process_results()
    if failures > 0:
        sys.exit(1)
    else:
        sys.exit(0)


class Harness(object):
    def __init__(self, build="Unspecified", log_dir=None,
                 log_level=logging.DEBUG, num_threads=1):
        self.harness_path = os.getcwd()
        self.build = build
        self.__setup_logging(log_dir, log_level, num_threads)
        self.dba = None # initialize with self.initialize_datbase()
        self.test_dir = "" # initialize with self.add_tests()
        self.file_to_dir = {} # initialize with self.add_tests()
        self.test_list = [] # initialize with self.add_tests()
        self.suite_id = 0 # recorded by self.__build_test_list_from_database()
        self.test_start_time = 0 # recorded by self.run_tests()
        self.test_end_time = 0 # recorded by self.run_tests()
        self.test_results = {} # populated by self.run_tests()
    
    def __setup_logging(self, dir, log_level, num_threads):
        # Set up the core logging engine
        logging.basicConfig(level=log_level,
                    format='%(asctime)s (%(thread)d) %(name)-24s %(levelname)-8s %(message)s',
                    datefmt='%Y-%m-%d %H:%M:%S')
        self.log = logging.getLogger("Harness")
        logging.addLevelName(100, "REPORT")
        
        # Set up the log file and log file rotation
        dir = os.path.abspath(dir)
        self.log_file = os.path.join(dir, "harness.log")
        rollover_count = 5
        if num_threads > 1:
            log_fmt = logging.Formatter("%(asctime)s (%(thread)d) %(name)-24s %(levelname)-8s %(message)s",
                                        "%Y-%m-%d %H:%M:%S")
        else:
            log_fmt = logging.Formatter("%(asctime)s %(name)-24s %(levelname)-8s %(message)s",
                                        "%Y-%m-%d %H:%M:%S")
        if not os.access(dir, os.W_OK):
            self.log.warning("There is no write access to the specified log "
                             "directory %s  No log file will be created." %dir)
        else:
            try:
                log_handle = logging.handlers.RotatingFileHandler(self.log_file, "w",
                                                                  100*1024*1024,
                                                                  rollover_count)
                log_handle.setFormatter(log_fmt)
                logging.getLogger("").addHandler(log_handle)
                if os.path.exists(self.log_file):
                    log_handle.doRollover()
            except:
                self.log.warning("Failed to rollover the log file.  The "
                                 "results will not be recorded.")
                logging.getLogger("").removeHandler(log_handle)
        self.log.log(100, " ".join(sys.argv))
    
    def initialize_database(self, ini):
        """ Attempts to create an initial connection to the database.  The
        configuration information is stored in a file, typically in the same
        directory as the harness, and is expected to be in the following format:
        [database]
        host: <host_name_of_SQL_server>
        db_name: <database_name>
        user: <MySQL_root_user_or_administrator_of_database>
        password: <password_for_above_user_name>
        
        Failure to connect to the database is only fatal if the list of tests
        is said to come from the database.
        """
        try:
            dir(MySQLdb)
        except NameError:
            self.dba = None
            self.log.warning("The MySQLdb package is not installed.  Tasks "
                             "requiring access to the database will fail.")
            return False
        config = configparser.SafeConfigParser()
        try:
            config.read(ini)
        except configparser.MissingSectionHeaderError:
            self.log.warning("The database configuration file %s is not in the "
                             "expected format.  It contains no section headers."
                             %ini)
            self.dba = None
            return False
        if not config.has_section("database"):
            self.log.warning("The database configuration file %s has no section "
                             "\"[database]\".  This section is required." %ini)
            self.dba = None
            return False
        try:
            host = config.get("database", "host").split()[0]
            user = config.get("database", "user").split()[0]
            password = config.get("database", "password").split()[0]
            db_name = config.get("database", "db_name").split()[0]
        except configparser.NoOptionError:
            self.log.warning("Unable to retrieve the required information from "
                             "the database configuration file.  Ensure that "
                             "the host, user, password, and datbase name are "
                             "specified.")
            self.dba = None
            return False
        
        # Create the database object.  It is not connected yet.  Any function 
        # using this needs to open and close it.
        try:
            self.dba = dbaccess(host, user, password, db_name)
            return True
        except:
            self.log.info("Unable to connect to the database.  Test results "
                          "will not be reported to the database.")
            self.dba = None
            return False
        
    def add_tests(self, test_dir, tests):
        """ This function takes test_dir (the directory in which all the files
        where the test cases may be found) and recursively lists out all of
        the files found.
        
        "tests" should be one of the following:
        * A suite from the database, from which a list of tests can be fetched.
        * A .csv file, containing a list of tests to run.
        * A directive in file:class format specifying a single test to run.
        
        Based on the pattern of "tests", and the contents of test_dir, a full
        listing of tests according to their file names are generated and
        stored as self.test_list.
        """
        file_dict = {}
        self.test_dir = os.path.abspath(test_dir)
        if not os.path.exists(self.test_dir):
            self.log.error("The test directory provided %s doesn't exist."
                           %self.test_dir)
            sys.exit(1)
        
        # Generate a list of all files in the test path
        for path, dirs, files in os.walk(self.test_dir):
            for f in files:
                file_dict[f] = path
        self.file_to_dir = file_dict
        
        if re.search("(?i)\.csv$", tests):
            test_list = self.__build_test_list_from_csv(tests)
        elif tests.find("py:") != -1:
            test_list = self.__build_test_list_from_cli(tests)
        else:
            test_list = self.__build_test_list_from_database(tests)
        if test_list == None:
            self.log.error("Unable to generate a test list from the provided "
                           "input: %s" %tests)
            sys.exit(1)
        self.test_list = test_list
        return True
    
    def __build_test_list_from_csv(self, csv_file):
        """ Creates a list of tests to run based on the file/class names found
        in the .csv file.
        """
        self.dba = None
        import csv
        test_list = []
        try:
            csv_file = open(csv_file, "r")
        except (OSError, IOError):
            self.log.error("Unable to open the test .csv file %s" %csv)
            return None
        
        tf = csv.reader(csv_file)
        for item in tf:
            if item == []: # probably an empty line or a DOS character
                continue
            if item[0][0] == "#": # the line is commented out
                continue
            if re.search("(?i)\.py", item[0]):
                test_list.append(item)
        csv_file.close()
        return test_list
    
    def __build_test_list_from_cli(self, cli):
        """ Creates a list of the test to run based on the file/class name
        specified at the command line.  It should be in file:class format, e.g.
        my_test_file.py:MyClassName
        """
        self.dba = None
        test_list = []
        try:
            file_name, class_name = cli.split(":")
            file_name = file_name.strip()
            class_name = class_name.strip()
            test_list.append([file_name, class_name, "", ""])
            return test_list
        except:
            return None
    
    def __build_test_list_from_database(self, suite):
        """ Creates a list of tests to run based on the file/class names found
        in the database.  The suite name is passed into the database through a
        query.  The database then returns the list of files/tests.
        """
        test_list = []
        if self.dba == None:
            self.log.error("A suite name was specified but no connection to "
                           "the test database could be made.")
            return None
        try:
            self.dba.connect_db()
        except:
            self.log.error("Unable to connect to database to check for the "
                           "suite named \"%s\".  Test run failed." %suite)
            sys.exit(1)
        # suite query.
        db_rc = self.dba.query("select * from auto_suite_list where suite_name=\"%s\"" %suite)
        if db_rc == ():
            self.log.error("The given suite \"%s\" is not in the autotest "
                           "database." %suite)
            self.dba.disconnect_db()
            sys.exit(1)

        # First get the suite ID from the query
        try:
            self.suite_id = db_rc[0]['id']
            self.log.debug("Running Suite ID %s: %s" %(str(self.suite_id),
                                                       suite))
        except:
            self.log.error("Failed to reference the suite ID %s.  The "
                           "following error was raised: %s"
                           %(str(self.suite_id), traceback.format_exc()))
            sys.exit(1)
        
        # If the suite's in the database check which tests are part of the suite
        db_rc = self.dba.query("select id, test_file, test_class from auto_test_suites where auto_suite_name=\"%s\"" %suite)
        if db_rc == ():
            self.log.error("Given suite \"%s\" does not have any tests "
                           "associated with it." %suite)
            self.dba.disconnect_db()
            sys.exit(1)

        # If we got the suite and we have tests in the suite, build the testList.
        # We need to query the test list to get other info, like xfail, skip values.
        atc_file = []
        atc_class = []
        atc_list = [] # Create a pair with File and Class, to allow same test name in different files.
        for item in db_rc:
            file_name = item['test_file']
            class_name = item['test_class']
            # Do a search on the auto_test_case list for test_file and test_class
            if file_name not in atc_file:
                atc_file.append(file_name)
            if class_name not in atc_class:
                atc_class.append(class_name)
            atc_list.append("%s:%s"%(file_name, class_name))
        atc_file = "\"%s\""%"\",\"".join(atc_file)
        atc_class = "\"%s\""%"\",\"".join(atc_class)
        db_test_data = self.dba.query("select File, Class, "
                                      "run_flag, run_as_root from "
                                      "auto_test_case where File in (%s) "
                                      "and Class in (%s) order by File, Class"
                                      %(atc_file, atc_class))
        if db_test_data == ():
            self.log.error("Failed to create test case list from the database.")
            self.dba.disconnect_db()
            sys.exit(1)
        # Update the testList with test_fileName, test_className, run_flag
        for item in db_test_data:
            if ("%s:%s" %(item['File'], item['Class']) in atc_list):
                test_list.append([item['File'], item['Class'],
                                 item['run_flag'], item['run_as_root']])
        return test_list
    
    def run_tests(self, config=None, store_to_database=False, 
                  stop_on_fail=False, iterations=1, threads=1):
        directives = profile.SuiteDirectives(config)
        if not directives.start():
            self.log.error("One of the pre-suite directives failed.  The tests "
                           "cannot be run.")
            sys.exit(1)
        self.test_start_time = int(time.time())
        
        ######################################################################
        # Single threaded running of tests
        if threads <= 1:
            for i in range(iterations):
                if iterations > 1:
                    self.log.info("Executing test iteration %s" %(i + 1))
                for item in self.test_list:
                    result = self.__execute_test_case(config, item)
                    if (result == False) and (stop_on_fail == True):
                        self.log.info("stop-on-fail is set.  Exiting.")
                        os._exit(1)
        
        ######################################################################
        # Multi-threaded test execution
        else:
            self.thread_list = []
            for i in range(iterations):
                if iterations > 1:
                    self.log.info("Executing test iteration %s" %(i + 1))
                for item in self.test_list:
                    tid = threading.Thread(None, self.__execute_test_case,
                                           "%s:%s" %(item[0], item[1]),
                                           (config, item))
                    self.thread_list.append(tid)
                    tid.start()
                    # If we are doing threading, when we hit the maximum
                    # number of threads, wait and let tests complete before
                    # starting more threads.
                    while len(self.thread_list) >= threads:
                        self.__check_threaded_tests(stop_on_fail)
                        if item == self.thread_list[-1]:
                            # If the test we just started is the last test, get 
                            # out of this while loop.
                            break
                        time.sleep(1)
                while len(self.thread_list) > 0:
                    self.__check_threaded_tests(stop_on_fail)
                    time.sleep(1)
        
        ######################################################################
        # Run the post-test check and report the results to the database.
        if not directives.end():
            self.log.error("One of the post-suite directives failed.  The "
                           "system may not have been restored to the same "
                           "state it was in when the test started.")
        self.test_end_time = int(time.time())
        if store_to_database:
            self.__update_database()
        return
    
    def __execute_test_case(self, config, test_case_params):
        test_status = {"test_file": "",
                       "test_class": "",
                       "run_flag": None,
                       "start_time": 0, # wall-clock time start
                       "end_time": 0, # wall-clock time end
                       "stime": 0, # kernel time duration
                       "utime": 0, # user time duration
                       "status": "FAIL",
                       "message": ""}
        rar_dict = {"": False,
                    0: False,
                    "0": False,
                    "False": False,
                    False: False,
                    None: False,
                    "disabled": False,
                    1: True,
                    "1": True,
                    "True": True,
                    True: True,
                    "enabled": True}
        
        test_config = copy.copy(config)
        test_status["test_file"] = test_case_params[0]
        test_status["test_class"] = test_case_params[1]
        test_status["run_flag"] = test_case_params[2]
        if len(test_case_params) > 3:
            try:
                test_config["run_as_root"] = rar_dict[test_case_params[3]]
            except KeyError:
                self.log.debug("run_as_root value for test %s:%s is set to '%s'."
                               %(test_status["test_file"],
                                 test_status["test_class"], test_case_params[3]))
                test_config["run_as_root"] = False
        
        #######################################################################
        # Various checks to go through before running the test.
        # If the test is set to "SKIP", dont' bother running it.
        if re.match("(?i)skip", test_status["run_flag"]):
            test_status["status"] = "SKIP"
            test_status["start_time"] = time.time()
            test_status["end_time"] = test_status["start_time"]
            self.__process_result(test_status)
            return True
        
        # Check the pre-test case directives.  This sets up the test 
        # environment.  If it fails, the test case will likely not run.
        try:
            directives = profile.TestCaseDirectives(self.file_to_dir[test_status["test_file"]],
                                                    test_config)
        except KeyError:
            self.log.error("The test file %s does not exist in the specified "
                           "test path %s"
                           %(test_status["test_file"], self.test_dir))
            test_status["status"] = "FAIL"
            self.__process_result(test_status)
            return False
            
        if not directives.start():
            self.log.error("One of the pre-test case directives failed.  The "
                           "test cannot be run.")
            directives.end()
            test_status["status"] = "FAIL"
            self.__process_result(test_status)
            return False
        
        # Make sure the test file is in the path so it can be imported.
        try:
            if self.file_to_dir[test_status["test_file"]] not in sys.path:
                sys.path.append(self.file_to_dir[test_status["test_file"]])
        except:
            self.log.error("Unable to find the test case %s in any file in the "
                           "test path %s." %(test_status["test_file"], self.test_dir))
            test_status["status"] = "FAIL"
            self.__process_result(test_status)
            return False
        
        # Import the test file and instantiate the test class.
        try:
            os.chdir(self.file_to_dir[test_status["test_file"]])
        except OSError:
            self.log.error("Unable to change into the directory %s.  "
                           "You must have execute permission on the "
                           "directory path.  Grant execute access to "
                           "world on the directories in the test path "
                           "or copy your tests into a directory in which "
                           "you do have execute permissions."
                           %self.file_to_dir[test_status["test_file"]])
            test_status["status"] = "FAIL"
            self.__process_result(test_status)
            return False
        self.log.debug("Running test %s:%s" %(test_status["test_file"],
                                              test_status["test_class"]))
        try:
            if sys.version_info[0] < 3:
                exec("import %s" %test_status["test_file"].split(".")[0])
                exec("test_object = %s.%s(test_config)"
                     %(test_status["test_file"].split(".")[0],
                       test_status["test_class"]))
            else:
                ldict = locals().copy()
                exec("import %s" %test_status["test_file"].split(".")[0],
                     globals())
                exec("test_object = %s.%s(test_config)"
                     %(test_status["test_file"].split(".")[0],
                       test_status["test_class"]), globals(), ldict)
                test_object = ldict["test_object"]
            os.chdir(self.harness_path)
        except AttributeError:
            self.log.error("The test case %s cannot be found in the file %s."
                           %(test_status["test_class"],
                             test_status["test_file"]))
            test_status["status"] = "FAIL"
            if os.getcwd() != self.harness_path:
                os.chdir(self.harness_path)
            self.__process_result(test_status)
            return False
        except:
            self.log.error("The following error occurred instantiating the "
                           "test class: %s" %traceback.format_exc())
            test_status["status"] = "FAIL"
            if os.getcwd() != self.harness_path:
                os.chdir(self.harness_path)
            self.__process_result(test_status)
            return False
        #######################################################################
        
        test_status["start_time"] = time.time()
        test_status["end_time"] = time.time()
        os_times = os.times()
        user_time_start = user_time_end = os_times[0]
        kernel_time_start = kernel_time_end = os_times[1]
        try:
            os.chdir(self.file_to_dir[test_status["test_file"]])
            if sys.version_info[0] < 3:
                exec("test_status[\"status\"] = test_object.run()")
            else:
                ldict = locals().copy()
                exec("test_status[\"status\"] = test_object.run()",
                     globals(), ldict)
                test_status["status"] = ldict["test_status"]["status"]
            del test_object
            os.chdir(self.harness_path)
        except AssertionError:
            self.log.error("The test case failed with an AssertionError and "
                           "will exit immediately.")
            os._exit(1)
        except:
            self.log.error("Handled exception %s" %traceback.format_exc())
            if os.getcwd() != self.harness_path:
                os.chdir(self.harness_path)
        os_times = os.times()
        user_time_end = os_times[0]
        kernel_time_end = os_times[1]
        test_status["stime"] = kernel_time_end - kernel_time_start
        test_status["utime"] = user_time_end - user_time_start
        test_status["end_time"] = time.time()
        
        if not directives.end():
            self.log.error("One of the post-test case directives failed.  The "
                           "test will be marked as failed.")
            test_status["status"] = "FAIL"
            self.__process_result(test_status)
            return False
        
        if not self.__process_result(test_status):
            return False
        return True
    
    def __check_threaded_tests(self, stop_on_fail):
        """ If we are running multiple thread, check if threads are done.  If
        it is, pop it from the thread_list. """
        for this_thread in self.thread_list:
            if not this_thread.isAlive():
                if stop_on_fail:
                        if self.test_results[this_thread.name]["status"] == "FAIL":
                            self.log.info("%s failed and stop-on-fail is set.  "
                                          "Exiting." %this_thread.name)
                            os._exit(1)
                self.thread_list.pop(self.thread_list.index(this_thread))

    def __process_result(self, test_status):
        """ This will process the test results.  The return value from the test 
        run could be of two types:  One is an Boolean for test pass or fail.  
        Two, a tuple or list with 2 values, first value is the boolean, second 
        value is a dictionary for test specific results. 
        """
        final_status = "FAIL"  # default to fail -- override with other status
        recorded_result = test_status["status"]
        string_extra = ""
        # Check what is the first or only return value, this value must be either
        # a text string of pass/fail or boolean True/False.  If testResult is 
        # a tuple/list, then the first value in the tuple/list is the pass/fail value.
        if isinstance(recorded_result, str):
            if re.match("(?i)pass", recorded_result):
                final_status = "PASS"
            elif re.match("(?i)skip", recorded_result):
                final_status = "SKIP"
        elif isinstance(recorded_result, bool):
            if recorded_result == True:
                final_status = "PASS"
        elif isinstance(recorded_result, (list, tuple)):
            # This is a list/tuple so the first value must be boolean or string.
            # Second value is expected to be a dictionary.
            if isinstance(recorded_result[0], bool):
                if recorded_result[0] == True:
                    final_status = "PASS"
            elif isinstance(recorded_result[0], str):
                if re.match("(?i)pass", recorded_result[0]):
                    final_status = "PASS"
            else:
                self.log.error("Unable to process tests result for %s:%s.  "
                               "Test status valued was recorded as %s"
                               %(test_status["test_file"],
                                 test_status["test_class"],
                                 recorded_result[0]))
            if not isinstance(recorded_result[1], dict):
                self.log.error("The second argument returned from the test "
                               "%s:%s is not a one-level dictionary."
                               %(test_status["test_file"],
                                 test_status["test_class"]))
            else:
                # Convert the dictionary to a flat string.
                for key, value in recorded_result[1].items():
                    try:
                        if type (value) == float:
                            # Float, need to convert to text first.
                            # Get upto 4 decimal points.
                            value = "%0.4f"%value
                        string_extra += "%s:%s," %(key, value)
                    except:
                        self.log.error("Unable to process test result "
                                       "dictionary into text, key:%s, value:%s"
                                       %(key, value))
            # Change string_extra to None if there are no items.
            if string_extra == "":
                string_extra = None
        test_status["test_data"] = string_extra
        if re.search("(?i)xfail", test_status["run_flag"]):
            # If the run is supposed to fail, and it did fail, the status is
            # set to XFAIL, i.e. a status indicating a known failure.
            if final_status == "FAIL":
                final_status = "XFAIL"
            else:
                self.log.error("Test %s:%s was expected to fail, but passed."
                               %(test_status["test_file"],
                                 test_status["test_class"]))
                final_status = "FAIL"
                
        test_status["status"] = final_status
        self.log.info("Test %s:%s: %s" %(test_status["test_file"],
                                         test_status["test_class"],
                                         test_status["status"]))
        self.test_results["%s:%s" %(test_status["test_file"],
                                    test_status["test_class"])] = test_status
        
        if test_status["status"] == "FAIL":
            return False
        else:
            return True
        
    def __update_database(self):
        job_id = 0
        if self.dba == None:
            return False
        try:
            self.dba.connect_db()
        except:
            self.log.error("The following exception was raised accessing the "
                           "database: %s" %traceback.format_exc())
            return False
        
        # insert into the job table and get back the insert ID, the insertID is the new job_id
        add_job_query = "insert into auto_test_jobs (start_time, end_time, status, test_host, suite_id) "+\
                        "values (%s, %s, '%s', '%s', '%s')" \
                        %(self.test_start_time, self.test_end_time, "done", get_local_ip(), self.suite_id)
        
        db_rc = self.dba.update(add_job_query)
        if db_rc != 0: # If we added at least 1 row to the table.
            job_id = self.dba.db.insert_id()  # The insert ID is the job_id, this is the primary ID field in the table.
        
        for label, test in self.test_results.iteritems():
            # 3: Compose the update line
            update_line = ""
            if test["start_time"] != None:
                update_line += "start_time=%f," %test["start_time"]
            if test["end_time"] != None:
                update_line += "end_time=%f," %test["end_time"]
                update_line += "run_time=%f," %(test["end_time"] - test["start_time"])
            if test["status"] != None:
                update_line += "test_status='%s'," %test["status"]
            if test["test_data"] != None:
                update_line += "test_data='%s'," %test["test_data"]
            update_line += "build='%s'," %self.build
            if test["stime"] != None:
                update_line += "stime=%f," %test["stime"]
            if test["utime"] != None:
                update_line += "utime=%f," %test["utime"]
            update_line = update_line.rstrip(",")
            query_line = "insert into auto_test_item set test_file='%s',test_class='%s',job_id=%s,%s" \
                        %(test["test_file"], test["test_class"], job_id, update_line)
            db_rc = self.dba.update(query_line)
            if db_rc == 0:
                self.log.error("Failed to update the database with data.")  
        
        self.dba.disconnect_db()
        return True

    def process_results(self):
        """ Take the test results dictionary and print it to screen. """
        if len(self.test_results) == 0:
            self.log.warning("No results recorded in test output.")
            return
        pass_count = 0
        fail_count = 0
        skip_count = 0
        xfail_count = 0
        total_count = len(self.test_results)
        keys = self.test_results.keys()
        if sys.version_info[0] < 3:
            keys.sort()
        else:
            keys = sorted(self.test_results.keys())
        
        final_output = "\r\n"
        final_output += "".ljust(80, "#")+"\r\n"
        final_output += "#"+"TEST RESULTS SUMMARY".center(78)+"#\r\n"
        final_output += "".ljust(80, "#")+"\r\n"
        
        current_file = ""
        for key in keys:
            value = self.test_results[key]["status"]
            file_name, class_name = key.split(":")
            if value.find("PASS") == 0:
                pass_count += 1
            elif value.find("FAIL") == 0:
                fail_count += 1
            elif value.find("SKIP") == 0:
                skip_count += 1
            elif value.find("XFAIL") == 0:
                xfail_count += 1
            if file_name != current_file:
                current_file = file_name
                final_output += "\n" + ("= " + file_name + "  ").ljust(80, "=") + "\r\n"
            final_output += ("  " + str(class_name) + " ").ljust(74, ".") + value + "\r\n"
        
        final_output += "\r\n"
        final_output += "Total run:    %s\r\n" %total_count
        final_output += "Total PASS:   %s\r\n" %pass_count
        final_output += "Total FAIL:   %s\r\n" %fail_count
        final_output += "Total SKIP:   %s\r\n" %skip_count
        final_output += "Total XFAIL:  %s\r\n" %xfail_count
        if (total_count != pass_count + fail_count + skip_count + xfail_count):
            final_output += "NOTE: Some tests didn't report results.\r\n\r\n"
        self.log.log(100, final_output)
        return fail_count

def intrupt_handle(sig, b):
    """ Exit if we get a SIGQUIT, SIGABRT, SIGINT, or SIGTERM. """
    print("\nCaught signal: %s.  Exiting." %str(sig))
    # Restore default signal handler
    if os.name == "posix":
        signal.signal(signal.SIGQUIT, signal.SIG_DFL)
    signal.signal(signal.SIGABRT, signal.SIG_DFL)
    signal.signal(signal.SIGINT, signal.SIG_DFL)
    signal.signal(signal.SIGTERM, signal.SIG_DFL)
    os._exit(1)
    
def get_local_ip():
    """ Return the IP address of the local machine. """
    import socket
    if sys.platform == "win32":
        try:
            return socket.gethostbyname(socket.gethostname())
        except:
            return "unknown"
    # On Linux, we will determine what interfaces exist.  We'll then try to
    # get the IP address assigned to the interface.  lo, the loopback interface,
    # will be omitted.
    try:
        import fcntl
        import struct
        net_file = open("/proc/net/dev", "r")
        proc_net_dev = net_file.read()
        net_file.close()
        find_what = re.compile(r"(\w+):", re.MULTILINE)
        all_nets = re.findall(find_what, proc_net_dev)
        for i in all_nets:
            if i == "lo":
                continue
            try:
                s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
                return socket.inet_ntoa(fcntl.ioctl(
                    s.fileno(),
                    0x8915,  # SIOCGIFADDR
                    struct.pack('256s', i[:15])
                )[20:24])
            # IOError gets raised if the interface doesn't have an IP address
            # assigned.  This is perfectly normal when a client has multiple
            # interfaces and some may not actually be in use currently.
            except IOError:
                continue
    except:
        return "unknown"
    return "unknown"


if __name__ == "__main__":
    # Setup the signal handlers to ensure quick/clean exit on these signals
    if os.name == "posix":
        signal.signal(signal.SIGQUIT, intrupt_handle)
    signal.signal(signal.SIGABRT, intrupt_handle)
    signal.signal(signal.SIGINT, intrupt_handle)
    signal.signal(signal.SIGTERM, intrupt_handle)
    logging.basicConfig(level=logging.DEBUG,
                        format='%(asctime)s %(name)-24s %(levelname)-8s %(message)s',
                        datefmt='%Y-%m-%d %H:%M:%S')
    main()
    
