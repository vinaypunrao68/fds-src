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

import string
import os
import sys
import requests
import csv
import random
import time

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
        sys.exit(-1)


class CapacityTest(object):
    '''
    Class to encapsulate the configuration and execution of a capacity
    test for the firebreak system.
    '''

    def __init__(self, interval=20, avg_size=512, std_dev=10, break_time=300, num_anoms=5):
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

    def __verify(self, start_time, end_time):
        '''
        Verifies that there were no anomolies between start_time and end_time
        '''
        # Open files

        # Verify the #s
        pass

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
        start_time = time.clock()

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
            rand_dev = random.randint(2, 6) # How many standard deviations from the avg
            rand = random.randint(0, rand_dev * self.std_dev)
            rand = (rand * -1) if random.randint(0, 1) == 0 else rand
            temp_data = os.urandom(self.avg_size + rand)
            res = requests.put(os.path.join(url, TestUtil.gen_random_name(6)), data=temp_data)
            if res.status_code != 200:
                print "An error has occurred, and an object could not be PUT!"
                sys.exit(-1)
            
            time.sleep(self.interval)
                
        end_time = time.clock()

        self.__verify(start_time, end_time)

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

    def __init__(self, avg_rate=10, std_dev=2, break_time=300, num_anoms=10):
        '''
        Params:
           avg_rate - int: average 'normal' request rate in seconds
           std_dev - int: standard deviation of normal request rate in seconds
           break_time - int: time after which the system should issue
                        requests at faster rate
           num_anoms - int: number of anomalous requests to make
        '''
        self.avg_rate = avg_rate
        self.std_dev = std_dev
        self.break_time = break_time
        self.num_anoms = num_anoms
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

    # Run capacity test
    # cap_test = CapacityTest()
    # cap_test.run('http://localhost:8000/abc')

    # Run performance test
    perf_test = PerfTest()
    perf_test.run('http://localhost:8000/abc')
