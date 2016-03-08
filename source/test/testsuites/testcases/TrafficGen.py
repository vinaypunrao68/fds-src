#!/usr/bin/python
#
# Copyright 2014 by Formation Data Systems, Inc.
#

'''
This file wraps the java trafficgen utility to provide a mechanism for longer running systems tests to 'bang harder'.
'''

# FDS test-case pattern requirements.
import unittest
import traceback
import TestCase

import os
import re
import subprocess

# This class wraps TrafficGen to provide test suite feedback
class TestTrafficGen(TestCase.FDSTestCase):
    def __init__(self, parameters=None, hostname=None, n_conns=None, exp_fail=None,
                 n_files=None, n_reqs=None, no_reuse=None, object_size=None,
                 outstanding_reqs=None, port=None, runtime=None, test_type=None,
                 timeout=None, token=None, username=None, volume_name=None):
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_TrafficGen,
                                             "Traffic Generation & Validation")


        # Find trafficgen
        fdscfg = self.parameters["fdscfg"]
        src_dir = fdscfg.rt_env.get_fds_source()
        self.om_node = fdscfg.rt_om_node
        self.fds_root = self.om_node.nd_conf_dict['fds_root']
        self.traffic_gen_dir = os.path.join(src_dir, 'Build/linux-x86_64.debug/tools/')
        self.traffic_gen_cmd = ['./trafficgen']
        self.traffic_gen_expect_failure = False

        self.traffic_gen_cmd.append('--fds-root=' + self.fds_root)

        if exp_fail is not None:
            self.traffic_gen_expect_failure = True
        if hostname is not None:
            self.traffic_gen_cmd.append('-hostname')
            self.traffic_gen_cmd.append(hostname)
        if n_conns is not None:
            self.traffic_gen_cmd.append('-n_conns')
            self.traffic_gen_cmd.append(n_conns)
        if n_files is not None:
            self.traffic_gen_cmd.append('-n_files')
            self.traffic_gen_cmd.append(n_files)
        if n_reqs is not None:
            self.traffic_gen_cmd.append('-n_reqs')
            self.traffic_gen_cmd.append(n_reqs)
        if no_reuse is not None and 'true' in no_reuse:
            self.traffic_gen_cmd.append('-no_reuse')
        if object_size is not None:
            self.traffic_gen_cmd.append('-object_size')
            self.traffic_gen_cmd.append(object_size)
        if outstanding_reqs is not None:
            self.traffic_gen_cmd.append('-outstanding_reqs')
            self.traffic_gen_cmd.append(outstanding_reqs)
        if port is not None:
            self.traffic_gen_cmd.append('-port')
            self.traffic_gen_cmd.append(port)
        if runtime is not None:
            self.traffic_gen_cmd.append('-runtime')
            self.traffic_gen_cmd.append(runtime)
        if test_type is not None:
            self.traffic_gen_cmd.append('-test_type')
            self.traffic_gen_cmd.append(test_type)
        if timeout is not None:
            self.traffic_gen_cmd.append('-timeout')
            self.traffic_gen_cmd.append(timeout)
        if token is not None:
            self.traffic_gen_cmd.append('-token')
            self.traffic_gen_cmd.append(token)
        if username is not None:
            self.traffic_gen_cmd.append('-username')
            self.traffic_gen_cmd.append(username)
        if volume_name is not None:
            self.traffic_gen_cmd.append('-volume_name')
            self.traffic_gen_cmd.append(volume_name)


    def test_TrafficGen(self):
        """
        Test Case:
        Run trafficgen with parameters provided and report back
        """

        # If we already have an auth token, use it
        if not '-token' in self.traffic_gen_cmd:
            # otherwise try to get one
            token = self.om_node.auth_token
            if token is not None:
                self.traffic_gen_cmd.append('-token')
                self.traffic_gen_cmd.append(token)

        # If we already have a bucket, use it
        if not '-volume_name' in self.traffic_gen_cmd:
            # otherwise try to get one
            if ('s3' in self.parameters and
                self.parameters['s3'].bucket1 is not None):
                self.traffic_gen_cmd.append('-volume_name')
                self.traffic_gen_cmd.append(self.parameters['s3'].bucket1.name)

        retval = True
        curr = os.getcwd()
        # We need to be in the ./Build/tools directory for trafficgen to work
        os.chdir(self.traffic_gen_dir)

        try:
            output = subprocess.check_output(self.traffic_gen_cmd)
            if self.traffic_gen_expect_failure:
                # Shouldn't be hitting this point
                self.log.error('TrafficGen expects to hit I/O error, but I/O seems to be working.')
                retval = False
        except subprocess.CalledProcessError as e:
            if self.traffic_gen_expect_failure:
                self.log.info('TrafficGen returned non zero error code, but this was expected.')
                retval = True
            else:
                self.log.error("CalledProcessError: {}. The return "
                               "code was: {}. Output was: {}".format(e.message, e.returncode, e.output))
                retval = False
        except OSError as e:
            self.log.error("OSError: {}. The return "
                           "code was: {}. Output was: {}".format(e.message, e.returncode, e.output))
            retval = False

        # Change back?
        os.chdir(curr)

        # Suppressing the output for now as it spams our logs
        #self.log.info(output)

        return retval


# This class runs trafficgen and only reports an error if the executable failed to run
# otherwise it will ALWAYS return success. This should be used for tests where IO is
# expected to run for some time and then fail, e.g. killing services during active IO
class RunTrafficGen(TestCase.FDSTestCase):
    def __init__(self, parameters=None, hostname=None, n_conns=None,
                 n_files=None, n_reqs=None, no_reuse=None, object_size=None,
                 outstanding_reqs=None, port=None, runtime=None,
                 test_type=None, timeout=None, token=None, username=None, volume_name=None):
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.run_TrafficGen,
                                             "Traffic Generation & Validation")


        # Find trafficgen
        fdscfg = self.parameters["fdscfg"]
        src_dir = fdscfg.rt_env.get_fds_source()
        self.om_node = fdscfg.rt_om_node

        self.traffic_gen_dir = os.path.join(src_dir, 'Build/linux-x86_64.debug/tools/')
        self.traffic_gen_cmd = ['./trafficgen']

        if hostname is not None:
            self.traffic_gen_cmd.append('-hostname')
            self.traffic_gen_cmd.append(hostname)
        if n_conns is not None:
            self.traffic_gen_cmd.append('-n_conns')
            self.traffic_gen_cmd.append(n_conns)
        if n_files is not None:
            self.traffic_gen_cmd.append('-n_files')
            self.traffic_gen_cmd.append(n_files)
        if n_reqs is not None:
            self.traffic_gen_cmd.append('-n_reqs')
            self.traffic_gen_cmd.append(n_reqs)
        if no_reuse is not None and 'true' in no_reuse:
            self.traffic_gen_cmd.append('-no_reuse')
        if object_size is not None:
            self.traffic_gen_cmd.append('-object_size')
            self.traffic_gen_cmd.append(object_size)
        if outstanding_reqs is not None:
            self.traffic_gen_cmd.append('-outstanding_reqs')
            self.traffic_gen_cmd.append(outstanding_reqs)
        if port is not None:
            self.traffic_gen_cmd.append('-port')
            self.traffic_gen_cmd.append(port)
        if runtime is not None:
            self.traffic_gen_cmd.append('-runtime')
            self.traffic_gen_cmd.append(runtime)
        if test_type is not None:
            self.traffic_gen_cmd.append('-test_type')
            self.traffic_gen_cmd.append(test_type)
        if timeout is not None:
            self.traffic_gen_cmd.append('-timeout')
            self.traffic_gen_cmd.append(timeout)
        if token is not None:
            self.traffic_gen_cmd.append('-token')
            self.traffic_gen_cmd.append(token)
        if username is not None:
            self.traffic_gen_cmd.append('-username')
            self.traffic_gen_cmd.append(username)
        if volume_name is not None:
            self.traffic_gen_cmd.append('-volume_name')
            self.traffic_gen_cmd.append(volume_name)


    def run_TrafficGen(self):
        """
        Test Case:
        Run trafficgen with parameters provided and report back
        """

        # If we already have an auth token, use it
        if not '-token' in self.traffic_gen_cmd:
            # otherwise try to get one
            token = self.om_node.auth_token
            if token is not None:
                self.traffic_gen_cmd.append('-token')
                self.traffic_gen_cmd.append(token)

        # If we already have a bucket, use it
        if not '-volume_name' in self.traffic_gen_cmd:
            # otherwise try to get one
            if ('s3' in self.parameters and
                self.parameters['s3'].bucket1 is not None):
                self.traffic_gen_cmd.append('-volume_name')
                self.traffic_gen_cmd.append(self.parameters['s3'].bucket1.name)

        curr = os.getcwd()
        # We need to be in the ./Build/tools directory for trafficgen to work
        os.chdir(self.traffic_gen_dir)

        try:
            output = subprocess.check_output(self.traffic_gen_cmd)
        except subprocess.CalledProcessError:
            self.log.info('TrafficGen returned non zero error code, but this was expected.')
        except OSError as e:
            self.log.error('TrafficGen didn\'t work! Something is very wrong: ' + e.message)
            return False

        # Change back?
        os.chdir(curr)

        # Suppressing the output for now as it spams our logs
        #self.log.info(output)

        return True
