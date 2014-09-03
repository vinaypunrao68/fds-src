'''TestUtils.py -- Test utility functions. These are largely a group
of static functions that are used throughout test scripts.
'''

import requests
import random
import string
import os
import re

class TestUtil(object):

    # Groups:
    # 0 ALL, 1 Volume, 2 Timestamp, 3 Puts, 4 Gets, 5 Queue Full
    # 6 % SSD access, Logical Bytes
    STREAMING_STATS_RE = re.compile(r'(?P<volume>.*),(?P<timestamp>\w{3} \w{3} \d{1,2} \d{2}:\d{2}:\d{2} \d{4}),\[ Puts: (?P<puts>\d*), Gets: (?P<gets>\d*), Queue Full: (?P<queue_full>\d*), Percent of SSD Accesses: (?P<ssd_acc>\d*), Logical Bytes: (?P<log_bytes>\d*), Physical Bytes: (?P<phys_bytes>\d*), Metadata Bytes: (?P<md_bytes>\d*), Blobs: (?P<blobs>\d*), Objects: (?P<objs>\d*), Ave Blob Size: (?P<avg_blob_size>\d*), Ave Objects per Blob: (?P<avg_obj_per_blob>\d*), Short Term Capacity Sigma: (?P<short_cap_sigma>\d*), Long Term Capacity Sigma: (?P<long_cap_sigma>\d*), Short Term Capacity WMA: (?P<short_cap_wma>\d*), Short Term Perf Sigma: (?P<short_perf_sigma>\d*), Long Term Perf Sigma: (?P<long_perf_sigma>\d*), Short Term Perf WMA: (?P<long_perf_wma>\d*),]')

    @staticmethod
    def gen_random_name(size):
        '''
        Generate a random sequence of characters and numbers to use as object/volume names.
        Params:
           size - int: size of string to generate
        Returns:
           random string of length size
        '''
        return ''.join(random.choice(string.ascii_uppercase + string.digits) for _ in range(size))

    @staticmethod
    def create_volume(url):
        '''
        Just creates a volume to ensure that we have something to write to
        Params:
           url - string: a url to PUT to
        Returns:
           True if volume created (or already exists); False if failure
        '''
        res = requests.put(url)
        if res.status_code != 200:
            for line in res.iter_lines():
                if 'BucketAlreadyExists' in line:
                    return True

            print "An error has occurred, and a volume could not be created!"
            print res
            return False
        
        return True

    @staticmethod
    def rnd_put(url, avg_size, std_dev):
        '''
        Generates PUT requests filled with avg_size +/- std_dev bytes.

        Params:
           url - string: a url to PUT to (should include bucket in URL)
           avg_size - int: Average size PUT to make
           std_dev - int: Standard deviation (# of bytes) randomness for each PUT
        Returns:
           Name of file PUT on success; None otherwise
        '''
        TestUtil.create_volume(url)

        # Make name
        name = TestUtil.gen_random_name(6)
        rand = random.randint((-1 * avg_size), std_dev)
        # Gen ~self.avg_size bytes of data within 1 std_dev
        temp_data = os.urandom(avg_size + rand)
        # Make PUT request
        req = requests.put(os.path.join(url, name), data=temp_data)
        if req.status_code != 200:
            print "An error has occurred, and the given object could not be PUT!"
            print req.text
            return None

        return name


    @staticmethod
    def get(url, valid_list):
        '''Generates a GET request issued to URL for all of the files from
        valid_list.

        Params:
           url - string: the url to make a request to (includes volume)
           valid_list - [string]: list of valid filenames to GET

        Returns:
           Requested GET data as a string; None on failure
        '''

        for get_val in valid_list:
            req = requests.get(os.path.join(url, get_val))
            if req.status_code != 200:
                print "An error has occurred, and the given object could not be GET!"
                print req.text
                return False

        return req.text

    @staticmethod
    def rnd_get(url, valid_list):
        '''Generates a GET request issued to URL for all of the files from
        valid_list.

        Params:
           url - string: the url to make a request to (includes volume)
           valid_list - [string]: list of valid filenames to GET

        Returns:
           Requested GET data as a string; None on failure
        '''
        
        get_val = random.choice(valid_list)

        req = requests.get(os.path.join(url, get_val))
        if req.status_code != 200:
            print "An error has occurred, and the given object could not be GET!"
            print req.text
            return False

        return req.text
