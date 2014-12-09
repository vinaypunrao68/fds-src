#!/usr/bin/python
# Copyright 2014 by Formation Data Systems, Inc.
# Written by Philippe Ribeiro
# philippe@formationds.com
import logging
import sys
import traceback
import unittest


class TestCase(unittest.TestCase):

    '''
    A particular System test is defined as a TestCase. Here, we define the
    common attributes that a particular test case must have. Therefore, all
    test cases inherit from TestCase
    '''
    logging.basicConfig(level=logging.INFO)
    logger = logging.getLogger(__name__)

    def __init__(self, name, parameters=None):
        """
        Attributes:
        -----------
        name : str
            the name of the test case
        parameters: object
            Given if any parameter should be given to the test
        """
        self.name = name
        super(TestCase, self).__init__()
        self.result = False

    def runTest(self):
        pass
