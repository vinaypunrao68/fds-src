#########################################################################
# @Copyright: 2015 by Formation Data Systems, Inc.                      #
# @file: test_multipart_upload.py                                                 #
# @author: Philippe Ribeiro                                             #
# @email: philippe@formationds.com                                      #
#########################################################################
import os
import sys
import logging

import config
import s3
import samples
import lib
import s3_volumes
import file_generator

class FileOps(object):
    '''
    Perform some basic file operations like download, hash for consistency
    and create sample test files.
    
    Attributes:
    ===========
    gb_size_files: Bool
        specify if the GB size files should be included.
    '''
    logging.basicConfig(level=logging.INFO)
    log = logging.getLogger(__name__)
    
    def __init__(self, gb_size_files=False):
        self.table = {}
        self.sample_files = []
        # add at least the MB size files
        self.create_sample_size_files(samples.sample_mb_files)
        if gb_size_files:
            # also add the GB size files
            self.create_sample_size_files(samples.sample_gb_files)
        
    def create_sample_size_files(self, files, source_path=config.TEST_DIR):
        '''
        Given a list of files, make sure those files exists in the location
        specified, plus store the file hash code.
        
        Attributes:
        ===========
        files: list
            a list of files to be hashed
        source_path: str
            the full path of the files source location. Default to config.TEST_DIR
        '''
        for f in files:
            path = os.path.join(source_path, f)
            if os.path.exists(path):
                self.sample_files.append(f)
                encode = lib.hash_file_content(path)
                self.table[f] = encode
                
    
    def download_files(self, bucket, source_path=config.DOWNLOAD_DIR):
        '''
        Given a list of files, make sure those files exists in the location
        specified, plus store the file hash code.
        
        Attributes:
        ===========
        bucket: obj
            a bucket object, to where this file must be upload to / download from
        source_path: str
            the full path of the files source location. Default to config.DOWNLOAD_DIR
        '''
        bucket_list = bucket.list()
        for obj in bucket_list:
            key_string = str(obj.key)
            path = os.path.join(source_path, key_string)
            if not os.path.exists(source_path):
                self.log.info("Downloading %s" % path)
                obj.get_contents_to_filename(source_path)
                
    def check_files_hash(self, source_path=config.DOWNLOAD_DIR):
        '''
        Check if the list of files previously hashed kept its consistency (hash
        value) throughout the upload / download.
        
        Attributes:
        ===========
        source_path: str
            the full path of the files source location. Default to config.DOWNLOAD_DIR
        
        Returns:
        =======
        bool: True if the file consistency is kept. Otherwise False.
        '''
        for k, v in self.table.iteritems():
            self.log.info("Hashing for file %s" % k)
            path = os.path.join(source_path, k)
            hashcode = lib.hash_file_content(path)
            if v != hashcode:
                self.log.warning("%s != %s" % (v, hashcode))
                return False
        return True
                