#!/usr/bin/python
# Copyright 2014 by Formation Data Systems, Inc.
# Written by Philippe Ribeiro
# philippe@formationds.com
import concurrent.futures as concurrent
import logging
import unittest

import config
import test_case


class TestSet(unittest.TestSuite):

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
    logger = logging.getLogger(__name__)

    def __init__(self, name, test_cases=[]):
        self.name = name
        self._tests = []
        # create all the test cases that belong to this test set.
        for tc in test_cases:
            # here it will actually be the actual test case, replaced by their
            # name, instead of the generic TestCase
            testcase = test_case.TestCase(name=tc)
            self.addTest(testcase)

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
                    self.logger.exception('An error occurrent for %s' % tc)
                else:
                    self.logger.info('Test %s completed successfully' % tc)
