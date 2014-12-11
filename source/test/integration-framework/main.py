#!/usr/bin/python
# Copyright 2014 by Formation Data Systems, Inc.
# Written by Philippe Ribeiro
# philippe@formationds.com
import ast
import argparse
import ConfigParser
import json
import logging
import os
import subprocess
import sys
import unittest
import xmlrunner

# Import the configuration file helper
import config
import testsets.test_set as test_set
import s3
import testsets.testcases.fdslib.BringUpCfg as bringup


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
        self.current_dir = os.path.dirname(os.path.realpath(__file__))
        self.log_dir = os.path.join(self.current_dir, config.log_dir)
        self.logger.info("Checking if the log directory")
        if not os.path.exists(self.log_dir):
            self.logger.info("Creating %s" % self.log_dir)
            os.makedirs(self.log_dir)
        # create the test suit runner
        self.runner = xmlrunner.XMLTestRunner(output=self.log_dir)
        # executes all the test sets listed in test_list.json
        for ts in test_sets_list:
            current_ts = test_set.TestSet(name=ts,
                                          test_cases=test_sets_list[ts])
            # create the test set directory
            testset_root = os.path.join(self.current_dir, config.test_sets)
            testset_path = os.path.join(testset_root, ts)
            self.logger.info("Checking if %s test set exists" % testset_path)
            if not os.path.exists(testset_path):
                self.logger.info("Creating %s" % testset_path)
                os.makedirs(testset_path)
            else:
                self.logger.info("%s already exists. Skipping." % testset_path)
                
            self.test_sets.append(current_ts)
        
    def __load_params(self):
        params = {}
        parser = ConfigParser.ConfigParser()
        parser.read(config.setup)
        sections = parser.sections()
        for section in sections:
            options = parser.options(section)
            for option in options:
                try:
                    params[option] = parser.get(section, option)
                    if params[option] == -1:
                        self.logger.info("skipping: %s" % option)
                except:
                    self.logger.info("Exception on %s!" % option)
                    params[option] = None
        #params['fdscfg'] = bringup.FdsConfigRun(None, None, None)
        #params['s3'] = s3.S3(None)
        return params
        
    def do_run(self):
        for ts in self.test_sets:
            self.logger.info("Executing Test Set: %s" % ts.name)
            self.runner.run(ts.suite)

    def do_work(self, target, args):
        '''
        The method do_work is used to execute test cases in parallel, in order
        to make them run faster.

        Arguments:
        ----------
        '''
        pass
    
    def test_progress(self):
        pass
        # @TODO: Philippe
        # I need to add a way to track test progress
        # and show those who passed, and those who failed

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
    print args
    operation = Operation(data['test_sets'])
    operation.do_run()

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Command line argument for'
                                     ' the integration framework')
    parser.add_argument('-f', '--failfast', action='store_true',
                        default=False,
                        help='Define if the test should fail fast.')
    parser.add_argument('-v', '--verbose', action='store_true',
                        default=False,
                        help='Define if output must be verbose.')
    parser.add_argument('-r', '--dryrun', action='store_true',
                         default=False,
                         help='Define if test must be run without' \
                         ' initial setup')
    parser.add_argument('-d', '--sudo-password', action='store_true',
                         default='passwd',
                         help='Define the root password, if not' \
                         ' specified defaults to "passwd"')
    parser.add_argument('-i', '--install', action='store_true',
                         default=False,
                         help='Specify if a fresh install must be' \
                         ' performed')
    args = parser.parse_args()
    main(args)
