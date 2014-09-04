'''fds-tiering-test.py -- Test tiering features by issuing a series
of puts/gets to create hotspots in data. There will be three major
tests:

1. Test the hybrid policy by writing data to a volume, and then
reading back a select subset of that data to create a hotspot. 
Verify that the frequently read data (hotspot) is moved to SSD
for faster access.

2. Set policy to SSD policy. Perform writes until SSDs are
full. Verify that once SSDs are full that further writes are sent to
HDD.

** !! This test requires the /fds/etc/platform.conf file's tiering
testing settings reflect values selected when testing. For example,
setting the SSD size to 5 objects means that an SSD policy should
write to HDD after 5 objects. !! **


3. Test performance by executing a series of parallel puts/gets with
both standard policy (HDD first), Hybrid Policy, and SSD
policy. Verify that the Hybrid/SSD policies perform better than the
standard HDD policy.
'''

import shlex
import subprocess
import argparse
import string
import os
import sys
import requests
import csv
import random
import time
import datetime
import re

sys.path.append('./fdslib/pyfdsp')
sys.path.append('./fdslib')

from SvcHandle import *
from TestUtils import TestUtil

class TierUtils(TestUtil):

    class Tiers(object):
        STANDARD = 0
        HYBRID = 1
        SSD = 2

    @staticmethod
    def single_hybrid_test(url, args):
        args = vars(args)
        del args['url']
        del args['func']
        
        for key, val in args.items():
            if val is None:
                del args[key]

        tt = TierTest(args['fds_root'])
        del args['fds_root']
        tt.run_hybrid_test(url, **args)

    @staticmethod
    def single_cap_test(url, args):
        args = vars(args)
        del args['url']
        del args['func']
        
        for key, val in args.items():
            if val is None:
                del args[key]

        tt = TierTest(args['fds_root'])
        del args['fds_root']
        tt.run_cap_pref_test(url, **args)


FDSCLI = '../Build/linux-x86_64.debug/bin/fdscli'

class TierTest(object):

    def __init__(self, fds_root, policy = TierUtils.Tiers.STANDARD):
        self.policy = policy
        self.fds_root = fds_root

        # Setup stuff for getting counters
        self.__svc_map = SvcMap("127.0.0.1", "7020")
        self.__svc_list = self.__svc_map.list()
        assert self.__svc_list != []

    def __find_log(self, url):
        '''
        Gets all of the stat logs from the directory logdir
        Params:
           url - string: url we're making requests to (used to determine volume name)
        Returns:
           list of log files to open
        '''
        # May need to filter out some log files, not sure if/how this
        # will be possible without opening each and reading first/last line
        full_path = os.path.join(self.fds_root, 'user-repo/vol-stats')        
        files = os.listdir(full_path)
        out = None
        for d in files:
            # These will be volume dirs
            logs_path = os.path.join(full_path, d)
            print "Logs path = ", logs_path
            # logs = os.listdir(logs_path)
            # for log in logs:
            # These will be logs within the log dirs
            vol_name = ''
            with open(os.path.join(logs_path, 'stat_min.log'), 'r') as fh:
                line = fh.readline()
                res = TestUtil.STREAMING_STATS_RE.search(line)
                if res is not None:
                    vol_name = res.group(1)
                
                v_name = os.path.basename(url)
                if v_name != vol_name:
                    break
                out = os.path.join(logs_path, 'stat_min.log')

        return out


    def run_hybrid_test(self, url, num_objs=300, obj_size=1024, hotspot_reads=100,
                        hotspot_size=10):
        '''This test will run by issuing a series of PUTs, to populate a
        volume, followed by a series of GETs. The GETs should continue
        to create a hotspot. When the hotspot occurs, the hotspot data
        should be moved to SSD, and requests satisfied from there.

        Params:
           num_objs - int: number of objects to write
           obj_size - int: size of objects to write
           hotspot_reads - int: number of reads to perform to create hotspot
           hotspot_size - int: number of objects to create hotspot with

        Returns:
           True if test passes; False otherwise
        '''
        # Create volume
        # TestUtil.create_volume(url)
        
        if url[-1] == '/':
            url = url[:-1]
        volume = os.path.basename(url)
        
        # Create policy & set policy using CLI
        # TODO(brian): Try to separate create and modify [not working as of 8/25/14]
        cmd = (FDSCLI + ' --volume-create ' + volume + ' -s 5000 ' + '-p 50 -M hybrid -i 1')
        cmd = shlex.split(cmd)
        res = subprocess.call(cmd)

        # Get a snapshot of initial stats
        # We just created the volume so we know all vals must be 0
        stats = {}

        # First do puts to populate the volume. Store all filenames.
        print "Populating volumes..."
        valid_objs = []
        for i in range(num_objs):
            valid_objs.append(TestUtil.rnd_put(url, obj_size, 0))

        # We need to get a snapshot of the stats here
        stats['post_write'] = self.__get_stat_snapshot_from_counters()
        
        # Now that we have the vol populated we should try to 
        # read back to create the hotspot
        print "Attempting to create hotspot..."
        # Pick hotspot_size objects from the pool
        hs_objs = set(random.sample(valid_objs, hotspot_size))
        remainder_objs = set(valid_objs) - hs_objs
        # Do the GETs
        for i in range(hotspot_reads):
            print "Performing GET #", i+1
            res = TestUtil.get(url, hs_objs)
        print "Hotspot should be created..."
        # Get a snapshot of stats here
        print "Collecting post-hotspot stats..."
        stats['post_hotspot'] = self.__get_stat_snapshot_from_counters()

        print "Sleeping for 3 minutes, allowing ranking engine to catch up..."
        for i in range(6):
            time.sleep(30)
            print "Collecting post-hotspot stats..."
            stats['post_hotspot'+str(i)] = self.__get_stat_snapshot_from_counters()



        print "Performing GETs on non-hot objects to verify data movement between HDD/SSD..."
        for obj in remainder_objs:
            res = TestUtil.get(url, [obj])

        # Get a snapshot of stats here
        print "Collecting post-test stats..."
        stats['post_test'] = self.__get_stat_snapshot_from_counters()
        print stats

        if (stats['post_write'][0] == num_objs and 
            stats['post_write'][1] is None and
            stats['post_test'][0] == num_objs and
            stats['post_hotspot'][1] == stats['post_test'][1]):
            print "Hybrid tier test: PASSED!"
            return True
        else:
            print "Hybrid tier test: FAILED!"
            return False


    def run_cap_pref_test(self, url, num_objs=300, obj_size=1024, hotspot_reads=100,
                          hotspot_size=10):
        '''This test will run by issuing a series of PUTs, to populate a
        volume, followed by a series of GETs. The GETs should continue
        to create a hotspot. When the hotspot occurs, the hotspot data
        should be moved to SSD, and requests satisfied from there.

        Params:
           num_objs - int: number of objects to write
           obj_size - int: size of objects to write
           hotspot_reads - int: number of reads to perform to create hotspot
           hotspot_size - int: number of objects to create hotspot with

        Returns:
           True if test passes; False otherwise
        '''
        # Create volume
        # TestUtil.create_volume(url)
        
        if url[-1] == '/':
            url = url[:-1]
        volume = os.path.basename(url)
        
        # Create policy & set policy using CLI
        # TODO(brian): Try to separate create and modify [not working as of 8/25/14]
        cmd = (FDSCLI + ' --volume-create ' + volume + ' -s 5000 ' + '-p 50 -M hybrid-prefcap -i 1')
        cmd = shlex.split(cmd)
        res = subprocess.call(cmd)

        # Get a snapshot of initial stats
        # We just created the volume so we know all vals must be 0
        stats = {}
        
        # First do puts to populate the volume. Store all filenames.
        print "Populating volumes..."
        valid_objs = []
        for i in range(num_objs):
            valid_objs.append(TestUtil.rnd_put(url, obj_size, 0))

        # We need to get a snapshot of the stats here
        stats['post_write'] = self.__get_stat_snapshot_from_counters()
        
        # Now that we have the vol populated we should try to 
        # read back to create the hotspot
        print "Attempting to create hotspot..."
        # Pick hotspot_size objects from the pool
        hs_objs = set(random.sample(valid_objs, hotspot_size))
        remainder_objs = set(valid_objs) - hs_objs
        # Do the GETs
        for i in range(hotspot_reads):
            print "Performing GET #", i+1
            res = TestUtil.get(url, hs_objs)
        print "Hotspot should be created..."
        print "Sleeping for 3 minutes, allowing ranking engine to catch up..."
        time.sleep(180)
        
        print "Performing GETs on non-hot objects to verify data movement between HDD/SSD..."
        for obj in remainder_objs:
            res = TestUtil.get(url, [obj])

        # Get a snapshot of stats here
        print "Collecting post-test stats..."
        stats['post_hotspot'] = self.__get_stat_snapshot_from_counters()
        print stats

        # [0] is puts [1] is gets
        if (stats['post_write'][0] == 0 and 
            stats['post_write'][1] is None and
            stats['post_hotspot'][0] == 0 and
            stats['post_hotspot'][1] == (hotspot_reads * len(hs_objs))):
            print "Prefer capacity tier test: PASSED!"
            return True
        else:
            print "Prefer capcity tier test: FAILED!"
            return False


    def __get_stat_snapshot_from_log(self, url):
        log_file = self.__find_log(url)
        
        with open(log_file, 'r') as fh:
            for line in fh:
                continue
            
        res = TestUtil.STREAMING_STATS_RE.match(line)
        if res is not None:
            return res.groupdict()
        
        return None
        
    def __get_stat_snapshot_from_counters(self):
        # Get counters from sm
        svc_uuid = ''
        for node in self.__svc_list:
            if ':sm' in node[0]:
                svc_uuid = long(node[0][:-3]) # Get just the UUID
                break
            
            assert svc_uuid != ''
        
        sm_counters = self.__svc_map.client(svc_uuid, 'sm').getCounters('*')
        print sm_counters
        puts_cnt = gets_cnt = None
        for key in sm_counters.keys():
            if ('put_ssd_obj.volume' in key and 'volume:0:' not in key):
                puts_cnt = sm_counters[key]
                
            if ('get_ssd_obj.volume' in key and 'volume:0:' not in key):
                gets_cnt = sm_counters[key]

        return (puts_cnt, gets_cnt)

    def __get_stat_snapshot(self, log_file):
        return self.__get_stat_snapshot_from_counters()
        
    def run_ssd_test(self):
        '''The ssd test will execute a series of PUTs which should all go to
        SSD. When SSD capacity is full further objects should go to
        HDD. After these PUTs a series of DELETEs will occur, removing
        objects that are on the SSD. With the free space, objects on
        HDD should be promoted to SSD.

        Params:
           ssd_size - int: size in 
        '''
        pass

    def run_capacity_prio_test(self):
        pass

    def run_perf_test(self):
        pass


if __name__ == "__main__":
    # Parse CLI args
    parser = argparse.ArgumentParser(description='Run tests for '
                                     'tiering policies')

    parser.add_argument('url', type=str, help='URL to send PUT/GET requests to. '
                        'This should include volume name, e.g. '
                        'http://localhost:8000/vol1')
    parser.add_argument('--fds-root', type=str, help='FDS root location. Default: /fds',
                        default='/fds')
    subparsers = parser.add_subparsers()
    hybrid_parser = subparsers.add_parser('hybrid', help='Run a hybrid tiering policy test.')
    hybrid_parser.set_defaults(func=TierUtils.single_hybrid_test)
    hybrid_parser.add_argument('--num_objs', type=int, 
                               help='Number of objects to populate volume with. Default: 300')
    hybrid_parser.add_argument('--obj_size', type=int,
                               help='Size in bytes of objects to PUT. Default: 1024')
    hybrid_parser.add_argument('--hotspot_reads', type=int,
                               help='Number of reads to perform to create a hotspot. Default: 100')
    hybrid_parser.add_argument('--hotspot_size', type=int,
                               help='Number of objects to create hotspot with. Should be < num_objs. '
                               'Default: 10')

    # ssd_parser = subparsers.add_parser('ssd', help='Run a ssd tiering policy test.')
    # ssd_parser.add_argument('--')

    
    # perf_parser = subparsers.add_parser('perf', help='Run a performance tiering '
    #                                    'test (compare policy performance).')
    cap_parser = subparsers.add_parser('capacity', help='Run a test with capacity preferred policy.')
    cap_parser.set_defaults(func=TierUtils.single_cap_test)
    cap_parser.add_argument('--num_objs', type=int, 
                               help='Number of objects to populate volume with. Default: 300')
    cap_parser.add_argument('--obj_size', type=int,
                               help='Size in bytes of objects to PUT. Default: 1024')
    cap_parser.add_argument('--hotspot_reads', type=int,
                               help='Number of reads to perform to create a hotspot. Default: 100')
    cap_parser.add_argument('--hotspot_size', type=int,
                               help='Number of objects to create hotspot with. Should be < num_objs. '
                               'Default: 10')

    # hdd_parser = subparsers.add_parser('hdd', help='Run a hdd tiering policy test.')
    
    args = parser.parse_args()
    args.func(args.url, args)
