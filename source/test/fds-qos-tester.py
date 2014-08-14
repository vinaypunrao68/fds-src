'''
fds-qos-tester.py - Test QOS functions: 
   - Single volume max IOPs
   - single volume min IOPs
   - multi-volume min IOPs
   - multi-volume min IOPs with IO
   - multi-volume max IOPs with IO

NOTE: Script assumes that it lives in the
<path-to-fds-src>/source/test directory
'''

import subprocess
import sys
import shlex
import re
import os
import string
import random
import pycurl
import tempfile
import time
import requests

sys.path.append('./fdslib/pyfdsp')
sys.path.append('./fdslib')

from SvcHandle import *

FDSCLI = '../Build/linux-x86_64.debug/bin/fdscli'

class FDS_Policy(object):
    ''' 
    Class to get/store per volume policies
    '''

    @property
    def policies(self):
        '''
        Just return the volumes and their policies
        '''
        return self._volumes

    @policies.setter
    def policies(self, value):
        self._volumes = value

    def __init__(self):
        self._volumes = {}
        
    def get_policies(self, domain='filler'):
        '''
        Run fdscli to get all active policies. This will construct a
        dictionary of {'volume name' : { policy attributes } }

        Params:
           None
        Returns:
           None
        '''
        
        # Issue the list volumes command to fdscli
        cmd = shlex.split(FDSCLI + ' --list-volumes ' + domain)
        res = subprocess.check_output(cmd, stderr=open(os.devnull))
        
        # Parse all of the good stuff out of the response
        vols_re = r'Volume (.*):(.*)\s*capacity (\d*) .{2}, iops_min (\d*), iops_max (\d*)'
        matches = re.finditer(vols_re, res)
        
        # Put it in a dictionary to return with
        vols = {}
        for match in matches:
            vols[match.group(1)] = {'id' : match.group(2),
                                    'capacity' : int(match.group(3)),
                                    'iops_min' : int(match.group(4)),
                                    'iops_max' : int(match.group(5)) }

        self._volumes = vols
        return vols

    def set_iops(self, volume, min_iops=None, max_iops=None):
        '''
        Sets the min/max IOPs for a volume.
        Params:
           volume - Name of the volume to modify
           max_iops - New maximum number of iops for volume
           min_iops - New minimum number of iops for volume
        Returns:
           None
        '''
        
        # Issue volume-modify command to fdscli
        if not self._volumes.has_key(volume):
            print "Modify volume failed, volume: " + volume + " does not exist!"
            sys.exit(1)

        if max_iops is None:
            max_iops = self._volumes[volume]['iops_max']
        if min_iops is None:
            min_iops = self._volumes[volume]['iops_min']

        cmd = (FDSCLI + ' --volume-modify ' + volume + ' -s 5000 -g '
               + str(min_iops) + ' -m ' + str(max_iops) + ' -r 1')
        cmd = shlex.split(cmd)

        res = subprocess.call(cmd)
        if 0 == res:
            self.get_policies()
            return True
        else:
            return False

    def create_volume(self, volume):
        '''
        Creates a volume with the specified name and max/min iops.
        Params:
           volume - Name of the volume to create
           max_iops - Maximum number of iops for the volume
           min_iops - Minimum number of iops for the volume
        Returns:
           True on success, False otherwise
        '''

        # Call FDS cli with the volume-create parameter
        cmd = FDSCLI + ' --volume-create ' + volume + ' -s 5000 -p 50 -i 1'
        cmd = shlex.split(cmd)

        res = subprocess.call(cmd, stdout=open(os.devnull), stderr=open(os.devnull))

        self.get_policies()
        
        if 0 == res:
            return True
        else:
            return False


class QOSTests(object):

    def __init__(self, min_iops=500, max_iops=700, num_vols=0):
        
        self.volumes = FDS_Policy()
        
        # Setup stuff for getting counters
        self.__svc_map = SvcMap("127.0.0.1", "7020")
        self.__svc_list = self.__svc_map.list()
        assert self.__svc_list != []

        # List of only my volumes (so we don't error report outside of
        # our context)
        self.my_vols = []

        for i in range(num_vols):
            rnd_name = ''.join(random.choice(string.ascii_letters + string.digits) for _ in range(6))
            # Create num_vols volumes with random names
            self.__create_volume(rnd_name)
            # Set policies on each
            self.__set_iops(rnd_name, min_iops, max_iops)

        # Get a list of volumes so we have it cached
        # This should only need to be updated when we
        # add/delete a volume
        self.volumes.get_policies()
        
    def list(self):
        return self.volumes.get_policies()


    def __set_iops(self, volume, min_iops=None, max_iops=None):
        return self.volumes.set_iops(volume, min_iops, max_iops)

    def __create_volume(self, volume):
        '''
        Creates a volume with the specified name and max/min iops.
        Params:
           volume - Name of the volume to create
        Returns:
           True on success, False otherwise
        '''

        res = self.volumes.create_volume(volume)
        if res:
            self.my_vols.append(volume)
            self.volumes.get_policies()
        return res

    def __do_io(self):
        '''
        Perform IO operations on all volumes. Record and return # IOPs
        achieved.
        Params:
           None
        Returns:
           A dict in the form {'vol_name' : iops_achieved}
        '''
        
        stats = {}
        # cURL in the file
        for volume in self.volumes.policies.keys():
            total_time = 0
            for i in range(1000): # Do multiple puts
                start = time.clock()
                rnd_name = ''.join(random.choice(string.ascii_letters + string.digits) for _ in range(6))
                requests.put('http://localhost:8000/' + volume + '/' + rnd_name,
                                  data=os.urandom(200000))
                requests.get('http://localhost:8000/' + volume + '/' + rnd_name)
                total_time += (time.clock() - start)
                
            # Get IOPs from AM using the perf counters
            svc_uuid = ''
            for node in self.__svc_list:
                if ':am' in node[0]:
                    svc_uuid = long(node[0][:-3]) # Get just the UUID
                    break
            
            assert svc_uuid != ''
        
            am_counters = self.__svc_map.client(svc_uuid, 'am').getCounters('*')
            puts_cnt = am_counters['am_put_obj_req.volume:'+
                                   str(long(self.volumes.policies[volume]['id'], base=16)) +
                                   ':volume=' +
                                   str(long(self.volumes.policies[volume]['id'], base=16)) +
                                   '.count']
            gets_cnt = am_counters['am_get_obj_req.volume:'+
                                   str(long(self.volumes.policies[volume]['id'], base=16)) +
                                   ':volume=' +
                                   str(long(self.volumes.policies[volume]['id'], base=16)) +
                                   '.count']
            
            stats[volume] = float(puts_cnt + gets_cnt) / total_time
 
        return stats
            
    def run_iops_test(self, assertions):

        '''
        Runs tests by performing writes on a volume and records the IOPs.
        Params:
           assertions - list of tuples of assertion, error message pairs to make about 
                        observed IOPs, eg:
                        [(lambda x: x > self.iops_min, "Min IOPs exceeded"), 
                         (lambda x: x < self.iops_max, "Max IOPs exceeded")]
        		to assert that x (observed IOPs) is within permissible ranges
        '''

        stats = self.__do_io()

        for assertion in assertions:
            for volume in self.my_vols:
                if volume in self.volumes.policies:
                    if not assertion[0](stats[volume]):
                        print "Error: " + assertion[1]
                        print stats[volume]
                else:
                    print "Error: Volume: " + volume + " not found."

    def run_policy_test(self):
        '''
        Tests for setting policies. These will be a static series of tests
        so will just run through them one by one.
        '''

        # Create volume
        self.__create_volume('test_vol1')
        # Assert that volume is created
        assert self.volumes.policies.has_key('test_vol1')
        # Set sane defaults
        self.__set_iops('test_vol1', 500, 1000)
        # Verify that settings are correct
        assert self.volumes.policies['test_vol1']['iops_min'] == 500
        assert self.volumes.policies['test_vol1']['iops_max'] == 1000
        print 'Volume create: PASSED!'
        
        # Single volume max IOPS > FDS MAX
        self.__set_iops('test_vol1', max_iops=50000)
        # Assert that the policy has not changed because we tried to
        # set it too high
        assert self.volumes.policies['test_vol1']['iops_max'] == 1000
        print 'Max IOPS > FDS MAX test: PASSED!'
        
        # Single volume min IOPS > FDS MAX
        self.__set_iops('test_vol1', min_iops=50000)
        # Assert that the policy has not changed
        assert self.volumes.policies['test_vol1']['iops_min'] == 500
        # set min > max
        self.__set_iops('test_vol1', min_iops=1150)
        assert self.volumes.policies['test_vol1']['iops_min'] == 500
        print 'Min IOPS > FDS MAX test: PASSED!'

        print "Running MULTI-VOLUME tests..."
        count = 2
        while 1:
            s_count = str(count)
            # Create additional volumes
            self.__create_volume('test_vol' + s_count)
            # If volume wasn't created, break out we've hit max IOPs
            if ('test_vol' + s_count) not in self.volumes.policies:
                break
            # Set sane defaults
            self.__set_iops('test_vol' + s_count, 500, 1000)
            # Verify settings are correct
            if self.volumes.policies['test_vol'+s_count]['iops_min'] < 500:
                print "Newest volume MIN IOPs could not be set!"
                print "This was the volume #" + s_count + "."
                break
            
            if self.volumes.policies['test_vol'+s_count]['iops_max'] < 1000:
                print "Newest volume MAX IOPs could not be set!"
                print "This was the volume #" + s_count + "."
                break

            if count > 100:
                print "Created 100 volumes and no limits reached. Possible error!"
                break
            count += 1
            
        print "Policy tests all PASSED!"

if __name__ == "__main__":

    #print "Single volume max IOPs test"
    #single_vol_max = QOSTests(1, 100, 1)
    #single_vol_max.run_test([(lambda x: x < single_vol_max.max_iops, "Max IOPs exceeded!"),
    #                      (lambda x: x > single_vol_max.min_iops, "Min IOPs not met!")])

    #print "Single volume min IOPs test"
    #single_vol_min = QOSTests(9999, 10000, 1)
    #single_vol_min.run_test([(lambda x: x, "!")])

    #print "Multi volume min IOPs test"
    #multi_vol = QOSTests(150, 300, 5)
    #multi_vol.run_iops_test([(lambda x: x < multi_vol.max_iops, "Max IOPs exceeded!"),
    #                         (lambda x: x >= multi_vol.min_iops, "Min IOPs not met!")])

    print "QOS policy management tests"
    policy_test = QOSTests()
    policy_test.run_policy_test()
