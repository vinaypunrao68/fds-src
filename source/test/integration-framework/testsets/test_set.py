#!/usr/bin/python
# Copyright 2014 by Formation Data Systems, Inc.
# Written by Philippe Ribeiro
# philippe@formationds.com
import concurrent.futures as concurrent
import importlib
import logging
import os
import unittest

import config
import testcase
import testcases
from symbol import parameters


class TestSet(object):

    '''
    In our definition, a TestSet object is a set of test cases for a particular
    feature.

    Attributes:
    -----------
    name : str
        the name of the test set being executed
    test_cases: list
        a list of all the test cases for this test set.
    '''
    logging.basicConfig(level=logging.INFO)
    log = logging.getLogger(__name__)

    def __init__(self, name, test_cases=[]):
        self.name = name
        self._tests = []
        self.suite = unittest.TestSuite()
        # create all the test cases that belong to this test set.
        for tc in test_cases:
            # here it will actually be the actual test case, replaced by their
            # name, instead of the generic TestCase
            fmodule = self.__process_module(test_cases[tc])
            module = importlib.import_module("testsets.testcases.%s" % fmodule)
            my_class = getattr(module, tc)
            instance = my_class(parameters=config.params)
            self.log.info("Adding test case: %s" % tc)
            self.log.info("Adding file name: %s" % test_cases[tc])
            self.suite.addTest(instance)

    def __process_module(self, module=""):
        '''
        This method checks if a file ends with .py extension (for now, but it
        could be extended to include a .java, .c or .cpp). If it does, then
        returns the module name (in Python file names are modules).
        
        Attributes:
        -----------
        module : str
            the file(module) name
            
        Returns:
        --------
        str : the file name stripped of the extension
        '''
        if module.endswith(".py"):
            return os.path.splitext(module)[0]
        else:
            raise ValueError("Only .py modules are supported.")
            sys.exit(1)

    def do_work(self, max_workers=5):
        '''
        Execute the test cases for this test set in a multithreaded fashion.
        User can specify the number of threads in max_workers, however the
        default is set to 5.

        Attributes:
        -----------
        max_workers : int
            specify the number of threads this program can run. The number of
            threads has to be between 1 and 20.

        Returns:
        -------
        None
        '''
        assert max_workers >= 1 and max_workers <= 20
        with concurrent.ThreadPoolExecutor(max_workers) as executor:
            tc_tests = {executor.submit(tc.runTest()):
                        tc for tc in self.test_cases}
            for tc in concurrent.as_completed(tc_tests):
                result = tc_tests[tc]
                try:
                    data = tc.result()
                except Exception as exc:
                    self.log.exception('An error occurrent for %s' % tc)
                else:
                    self.log.info('Test %s completed successfully' % tc)
