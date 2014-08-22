'''
fds-firebreak-tester.py -- Test firebreak metrics.  This script
will run two tests. The first will issue a number of puts with files
of a similar (small) size. After a number of these puts (configurable)
a series of large puts will be issued to determine if our metrics are
working in regards to capacity.

The second test will issue both puts and gets in some consistent 
pattern, followed by a series of puts/gets that are inconsistent
with the previous series. This should trigger the metrics in the perf
counts to change.
'''

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
import functools

class TestUtil(object):

    @staticmethod
    def gen_random_name(size):
        return ''.join(random.choice(string.ascii_uppercase + string.digits) for _ in range(size))

    @staticmethod
    def create_volume(url):
        '''
        Just creates a volume to ensure that we have something to write to
        Params:
           url - string: a url to PUT to
        Returns:
           None
        '''
        res = requests.put(url)
        if res.status_code != 200:
            for line in res.iter_lines():
                if 'BucketAlreadyExists' in line:
                    return

            print "An error has occurred, and a volume could not be created!"
            print res
            sys.exit(-1)

    @staticmethod
    def single_cap_run(url, args):
        args = vars(args)
        del args['url']
        del args['func']

        for key, val in args.items():
            if val is None:
                del args[key]

        cap_run = CapacityTest(**args)
        cap_run.run(url)

    @staticmethod
    def single_perf_run(url, args):
        args = vars(args)
        del args['url']
        del args['func']

        for key, val in args.items():
            if val is None:
                del args[key]
        
        perf_run = PerfTest(**args)
        perf_run.run(url)


NAME_TS_RE = re.compile(r'(.*),(\d*),')
STCS_RE = re.compile(r'Short term capacity sigma: (\d*.\d*)')
LTCS_RE = re.compile(r'Long term capacity sigma: (\d*.\d*)')
STCW_RE = re.compile(r'Short term capacity wma: (-?\d*.\d*)')
STPS_RE = re.compile(r'Short term perf sigma: (\d*.\d*)')
LTPS_RE = re.compile(r'Long term perf sigma: (\d*.\d*)')
STPW_RE = re.compile(r'Short term perf wma: (-?\d*.\d*)')

STATS_RE = re.compile(r'(.*),(.*),\[.* Short term capacity sigma: (\d*.\d*), Long term capacity sigma: (\d*.\d*), Short term capacity wma: (-?\d*.\d*), Short term perf sigma: (\d*.\d*), Long term perf sigma: (\d*.\d*), Short term perf wma: (-?\d*.\d*)')

class CapacityTest(object):
    '''
    Class to encapsulate the configuration and execution of a capacity
    test for the firebreak system.
    '''

    # TODO: Add additional params such as std_dev of anom requests

    def __init__(self, interval=20, avg_size=512, std_dev=10, break_time=300, num_anoms=10,
                 fds_root='/fds'):
        '''
        Defaults to 20 second interval, 5 minute break time, average size
        of 512 bytes, with 10 byte variance

        Params:
           interval - int: interval in seconds
           avg_size - int: average size of PUT request data in bytes
           std_dev - int: Standard deviation in bytes
           break_time - int: time after which the anomalous activity should occur
           num_anoms - int: number of times to send an anomalous PUT
        '''
        self.interval = interval
        self.avg_size = avg_size
        self.std_dev = std_dev
        self.break_time = break_time
        self.num_anoms = num_anoms
        self.fds_root = fds_root

    def __find_logs(self, start_time, end_time, url):
        '''
        Gets all of the stat logs from the directory logdir
        Params:
           start_time - int: timestamp of start of script run
           end_time - int: timestamp of end of script run
           url - string: url we're making requests to (used to determine volume name)
        Returns:
           list of log files to open
        '''
        # May need to filter out some log files, not sure if/how this
        # will be possible without opening each and reading first/last line
        full_path = os.path.join(self.fds_root, 'user-repo/vol-stats')        
        files = os.listdir(full_path)
        out = []
        for d in files:
            # These will be volume dirs
            logs_path = os.path.join(full_path, d)
            logs = os.listdir(logs_path)
            for log in logs:
                # These will be logs within the log dirs
                with open(os.path.join(logs_path, log), 'r') as fh:
                    line = fh.readline()
                    res = NAME_TS_RE.search(line)
                    if res is not None:
                        vol_name = res.group(1)
                        ts = int(res.group(2))
                    else:
                        continue

                    v_name = os.path.basename(url)
                    if v_name != vol_name:
                        break
                    out.append(os.path.join(logs_path, log))
                            
        return out
        

    def __verify(self, start_time, end_time, url):
        '''
        Compares log numbers vs script settings to ensure expected values
        Params:
           start_time - int: The start time of the script
           end_time - int: The end time of the script run
        '''
        # Open files
        stats = {}
        files = self.__find_logs(start_time, end_time, url)
        for f in files:
            with open(f, 'r') as fh:
                for line in fh:
                    res = STATS_RE.match(line)
                    if res is not None:
                        # Prune log entries that aren't within our window
                        ts = datetime.datetime.fromtimestamp(res.group(3))
                        if start_time <= ts <= end_time:
                            # We only need the first 3 groups as those are capacity #s
                            stats[res.group(3)] = (res.group(4), res.group(5),
                                                   res.group(6))
        # Verify the #s
        
        # The sigma from 0 to break time should be = self.std_dev
        # The sigma from break time until num_anoms * interval should be > previous sigma
    
    def run(self, url):
        '''
        Generates PUT requests filled with ~self.avg_size bytes. Sends
        these requests at self.interval interval. After break_time, 
        a PUT much larger than avg_size will be issued.

        Params:
           url - string: a url to PUT to (should include bucket in URL)
        Returns:
           None
        '''
        TestUtil.create_volume(url)
        
        # Figure out how many requests to make based on interval/break_time
        num_reqs = self.break_time / self.interval
        start_time = time.time()

        for i in range(num_reqs):
            print "Putting request #" + str(i)
            # Generate a random deviation within 1 std_dev
            rand = random.randint((-1 * self.std_dev), self.std_dev)
            # Gen ~self.avg_size bytes of data within 1 std_dev
            temp_data = os.urandom(self.avg_size + rand)
            # Make PUT request
            req = requests.put(os.path.join(url, TestUtil.gen_random_name(6)), data=temp_data)
            if req.status_code != 200:
                print "An error has occurred, and the given object could not be PUT!"
                print req.text
                sys.exit(-1)

            time.sleep(self.interval)
            
        # If we've hit this point then its time for the break
        for i in range(self.num_anoms):
            print "Putting anomalous request #" + str(i)
            # rand_dev = random.randint(2, 6) # How many standard deviations from the avg
            rand_dev = 1000
            temp_data = os.urandom(self.avg_size + (rand_dev * self.std_dev))
            res = requests.put(os.path.join(url, TestUtil.gen_random_name(6)), data=temp_data)
            if res.status_code != 200:
                print "An error has occurred, and an object could not be PUT!"
                sys.exit(-1)
            
            time.sleep(self.interval)
                
        end_time = time.time()

        self.__verify(start_time, end_time, url)

class PerfTest(object):
    '''
    Class to encapsulate the configuration and execution of a performance
    test for the firebreak system.
    This particular test case will issue a series of PUT/GET requests at
    a stable interval. This interval will be random within 1 standard
    deviation of the average interval. After break_time it will issue
    a series of PUT/GET requests that occur at a faster rate, causing
    the firebreak.
    '''

    def __init__(self, avg_rate=10, std_dev=2, break_time=300, anom_rate=1, num_anoms=10):
        '''
        Params:
           avg_rate - int: average 'normal' request rate in seconds
           std_dev - int: standard deviation of normal request rate in seconds
           break_time - int: time after which the system should issue
                        requests at faster rate
           anom_rate - int: rate at which anomalous requests will be sent
           num_anoms - int: number of anomalous requests to make
        '''
        self.avg_rate = avg_rate
        self.std_dev = std_dev
        self.break_time = break_time
        self.num_anoms = num_anoms
        self.anom_rate = anom_rate
        self.__valid_gets = []
        
    def __make_random_request(self, url):
        # Decide whether GET or PUT
        req_type = random.choice(['GET', 'PUT'])
        if req_type == 'GET':
            res = requests.get(os.path.join(url, random.choice(self.__valid_gets)))
            if res.status_code != 200:
                print "An error has occurred, and the given object could not be GET!"
                print req.text
                sys.exit(-1)
        else:
            # Gen 512 bytes of data
            temp_data = os.urandom(512)
            # Make PUT request
            rand_name = TestUtil.gen_random_name(6)
            res = requests.put(os.path.join(url, rand_name), data=temp_data)
            if res.status_code != 200:
                print "An error has occurred, and the given object could not be GET!"
                print req.text
                sys.exit(-1)
            self.__valid_gets.append(rand_name)
        
    def run(self, url):
        '''
        Runs a performance based firebreak test.
        Params:
           url - string: the base url of the system
        Returns:
           None
        '''
        TestUtil.create_volume(url)

        # Do some initial setup to ensure that we've got items to request
        for i in range(50):
            temp_data = os.urandom(512)
            # Make PUT request
            name = TestUtil.gen_random_name(6)
            req = requests.put(os.path.join(url, name), data=temp_data)
            if req.status_code != 200:
                print "An error has occurred, and the given object could not be PUT!"
                print req.text
                sys.exit(-1)
            self.__valid_gets.append(name)
            
        # Figure out how many requests to make based on interval/break_time
        num_reqs = self.break_time / self.avg_rate
        start_time = time.clock()

        # Do the requests
        for i in range(num_reqs):
            print "Sending request #" + str(i)
            self.__make_random_request(url)
            
            # Generate a random deviation within 1 std_dev
            rand_wait = random.randint((-1 * self.std_dev), self.std_dev)
            time.sleep(self.avg_rate + rand_wait)
            
        # If we've hit this point then its time for the break
        for i in range(self.num_anoms):
            print "Putting anomalous request #" + str(i)

            rand_dev = random.randint(2, 6) # How many standard deviations from the avg
            interval = self.avg_rate - (self.std_dev * rand_dev)
            if interval < 0:
                interval = 0
            self.__make_random_request(url)
            time.sleep(interval)
                
        end_time = time.clock()



if __name__ == "__main__":

    # Parse CLI args
    parser = argparse.ArgumentParser(description='Run tests for '
                                     'performance or capacity firebreaks.')

    parser.add_argument('url', type=str, help='URL to send PUT/GET requests to. This should include volume name, e.g.'
                        'http://localhost:8000/vol1')
    parser.add_argument('--fds-root', type=str, help='FDS root location.', default='/fds')
    
    subparsers = parser.add_subparsers()
    cap_parser = subparsers.add_parser('capacity', help='Run a capacity firebreak test.')
    cap_parser.add_argument('--interval', type=int, help='Interval in seconds between requests. Default: 20')
    cap_parser.add_argument('--avg_size', type=int, help='Average object size. Default: 512')
    cap_parser.add_argument('--std_dev', type=int, help='Standard deviation of normal requests. Default: 10')
    cap_parser.add_argument('--break_time', type=int, help='Time in seconds after which firebreak should occur. Default: 300')
    cap_parser.add_argument('--num_anoms', type=int, help='Number of anomalous requests to make after break_time.'
                            ' These will occur at interval time. Default: 10')
    cap_parser.set_defaults(func=TestUtil.single_cap_run)

    perf_parser = subparsers.add_parser('performance', help='Run a performance firebreak test.')
    # avg_rate=10, std_dev=2, break_time=300, num_anoms=10
    perf_parser.add_argument('--avg_rate', type=int, help='Average rate in seconds to send requests. Default: 10')
    perf_parser.add_argument('--std_dev', type=int, help='Standard deviation in seconds in request rate. Default: 2')
    perf_parser.add_argument('--break_time', type=int, help='Number of seconds after which firebreak should occur. Default: 300')
    perf_parser.add_argument('--anom_rate', type=int, help='Anomalous request rate in seconds. Default: 1')
    perf_parser.add_argument('--num_anoms', type=int, help='Number of anomalous requests to make. Default: 10')
    perf_parser.set_defaults(func=TestUtil.single_perf_run)
    
    args = parser.parse_args()
    print args
    args.func(args.url, args)
