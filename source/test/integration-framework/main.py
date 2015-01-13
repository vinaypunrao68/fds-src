#!/usr/bin/python
# Copyright 2014 by Formation Data Systems, Inc.
# Written by Philippe Ribeiro
# philippe@formationds.com
import argparse
import concurrent.futures as concurrent
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
import fds
import multinode
import testsets.test_set as test_set
import s3


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

    def __init__(self, test_sets_list, args):
        self.test_sets = []
        self.multicluster = None
        self.args = args
        self.current_dir = os.path.dirname(os.path.realpath(__file__))
        self.log_dir = os.path.join(self.current_dir, config.log_dir)
        self.logger.info("Checking if the log directory exists...")
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
        return params
    
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
        for ts in self.test_sets:
                    self.logger.info("Executing Test Set: %s" % ts.name)
                    self.runner.run(ts.suite)
        assert max_workers >= 1 and max_workers <= 20
        with concurrent.ThreadPoolExecutor(max_workers) as executor:
            ts_tests = {executor.submit(self.runner.run(ts.suite)):
                        ts for ts in self.test_sets}
            for ts in concurrent.as_completed(ts_tests):
                result = ts_tests[ts]
                try:
                    data = True
                except Exception as exc:
                    self.logger.exception('An error occurrent for %s' % ts)
                else:
                    self.logger.info('Test %s completed successfully' % ts)
                
    def do_run(self):
        '''
        Run all the test suits presented in this framework, but first check if
        all the fds processes are running.
        '''
        if self.args.test == 'single':
            self.fds_node = fds.FDS()
            if not self.fds_node.check_status():
                self.fds_node.start_single_node()
        elif self.args.test == 'multi':
            if self.args.type == "aws":
                if self.args.name == None:
                    raise ValueError, "A name tag must be given to the AWS" \
                                      "cluster"
                self.multicluster = multinode.Multinode(name=self.args.name, 
                                              instance_count=self.args.count,
                                              type=self.args.type)
            else:
                # make the baremetal version the default one.
                self.multicluster = multinode.Multinode(type=self.args.type,
                                              inventory=self.args.inventory)
        for ts in self.test_sets:
            self.logger.info("Executing Test Set: %s" % ts.name)
            self.runner.run(ts.suite)
        
        # After completion, assert the FDS-related processes are stopped.
        self.do_stop()

    def do_stop(self):
        '''
        Stop the cluster provision, whether it is a multinode AWS cluster,
        or a single cluster instance.
        '''
        if self.args.test == 'single' or self.args.test is None:
            self.fds_node.stop_single_node()
        elif self.args.test == 'multi':
            if self.multicluster is not None:
                self.multicluster.destroy_cluster()
        
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
    test_file = None
    if args.config_file is None:
        test_file = config.test_list
    else:
        test_file = args.config_file
        
    with open(test_file, 'r') as data_file:
        data = json.load(data_file)

    if 'test_sets' not in data:
        raise ValueError("test_sets are required in the %s file" %
                         config.test_list)
        sys.exit(2)
    operation = Operation(data['test_sets'], args)
    operation.do_run()

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Command line argument for'
                                     ' the integration framework')
    #parser.add_argument('-f', '--failfast', action='store_true',
    #                    default=False,
    #                    help='Define if the test should fail fast.')
    #parser.add_argument('-v', '--verbose', action='store_true',
    #                    default=False,
    #                    help='Define if output must be verbose.')
    #parser.add_argument('-r', '--dryrun', action='store_true',
    #                     default=False,
    #                     help='Define if test must be run without' \
    #                     ' initial setup')
    #parser.add_argument('-d', '--sudo-password', action='store_true',
    #                     default='passwd',
    #                     help='Define the root password, if not' \
    #                     ' specified defaults to "passwd"')
    #parser.add_argument('-i', '--install', action='store_true',
    #                     default=False,
    #                     help='Specify if a fresh install must be' \
    #                     ' performed')
    parser.add_argument('-b', '--build', action='store_true',
                        default='nightly',
                        help='Specify if the build is local or nightly')
    parser.add_argument('-c', '--count',
                        default=1,
                        help='Specify how many nodes will go to a multi node' \
                        ' cluster.')
    parser.add_argument('-t', '--type',
                        default='baremetal',
                        help='Specify if the cluster will be created using' \
                        ' AWS or baremetal instances.')
    parser.add_argument('-u', '--inventory',
                        default=None,
                        help='Define if user wants to use a different' \
                        ' inventory than default.')
    parser.add_argument('-x', '--test',
                        default='single',
                        help='Define if framework should run for single' \
                        ' or multi node cluster')
    parser.add_argument('-n', '--name',
                        default=None,
                        help='Specify a name of the cluster, if AWS a tag ' \
                        'name must be given.')
    parser.add_argument('-f', '--config_file',
                        default=None,
                        help='User can specify which config file will be ' \
                        'used. The config file has to be .json.')
    args = parser.parse_args()
    main(args)
