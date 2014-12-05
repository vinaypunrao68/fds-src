#!/usr/bin/python
# Copyright 2014 by Formation Data Systems, Inc.
# Written by Philippe Ribeiro
# philippe@formationds.com
import argparse
import json
import logging
import os
import subprocess
import sys

# Import the configuration file helper
import config
import testsets.test_set as test_set

from pprint import pprint


class Operation(object):

    '''
    The class Operation is responsible for creating all the test sets, with
    their respective test cases, and execute them in parallel using the do_run
    method.

    Arguments:
    ----------
    test_set_list : dict
        A json dict that maps the test set to its test cases
    '''

    # establish the basic logging operations
    logging.basicConfig(level=logging.INFO)
    logger = logging.getLogger(__name__)

    def __init__(self, test_sets_list):
        self.test_sets = []
        for ts in test_sets_list:
            current_ts = test_set.TestSet(name=ts,
                                          test_cases=test_sets_list[ts])
            self.test_sets.append(current_ts)

    def do_run(self):
        for ts in self.test_sets:
            self.logger.info("Executing Test Case: %s" % ts.name)

    def do_work(self, target, args):
        '''
        The method do_work is used to execute test cases in parallel, in order
        to make them run faster.

        Arguments:
        ----------
        '''
        pass


def main(args):
    '''
    Define the main method for the Integration Test Framework.
    This method will be responsible for execute all the system tests in earnest

    Arguments:
    ----------
    args : Dict
        a dictionary between the command line arguments and its values

    Returns:
    --------
    None
    '''
    with open(config.test_list, 'r') as data_file:
        data = json.load(data_file)

    if 'test_sets' not in data:
        raise ValueError("test_sets are required in the %s file" %
                         config.test_list)
        sys.exit(2)

    operation = Operation(data['test_sets'])
    operation.do_run()

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Command line argument for'
                                     ' the integration framework')
    args = parser.parse_args()
    main(args)
