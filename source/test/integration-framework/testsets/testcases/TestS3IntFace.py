#!/usr/bin/python
#
# Copyright 2014 by Formation Data Systems, Inc.
#

# FDS test-case pattern requirements.
import __init__

import boto
import filecmp
import math
import os
import sys
import traceback
import unittest

from filechunkio import FileChunkIO
from boto.s3 import connection
from boto.s3.key import Key

import config
import testsets.testcase as testcase

# Class to contain S3 objects used by these test cases.
class S3(object):
    def __init__(self, conn):
        self.conn = conn
        self.bucket1 = None
        self.keys = []  # As we add keys to the bucket, add them here as well.


# This class contains the attributes and methods to test
# the FDS S3 interface to create a connection.
#
# You must have successfully retrieved an authorization token from
# OM and stored it in fdscfg.rt_om_node.auth_token. 
# See TestOMIntFace.TestGetAuthToken.
class TestS3GetConn(testcase.FDSTestCase):
    
    def __init__(self, parameters=None):
        super(TestS3GetConn, self).__init__(parameters)

    def runTest(self):
        self.test_passed = True

        if self.test_failure:
            self.log.warning("Skipping Case %s. stop-on-fail/failfast set " \
                             "and a previous test case has failed." %
                             self.__class__.__name__)
            return unittest.skip("stop-on-fail/failfast set and a previous " \
                                 "test case has failed.")
        else:
            self.log.info("Running Case %s." % self.__class__.__name__)

        try:
            if not self.test_S3GetConn():
                self.test_passed = False
        except Exception as inst:
            self.log.error("Getting an S3 connection caused exception:")
            self.log.error(traceback.format_exc())
            self.log.error(inst.message)
            self.test_passed = False

        super(self.__class__, self).reportTestCaseResult(self.test_passed)

        # If there is any test fixture teardown to be done, do it here.
        self.assertTrue(self.test_passed)


    def test_S3GetConn(self):
        """
        Test Case:
        Attempt to get an S3 connection.
        """
        assert(self.parameters["s3"].conn)
        return True


# This class contains the attributes and methods to test
# the FDS S3 interface to create a bucket.
#
# You must have successfully created an S3 connection
# and stored it in self.parameters["s3"].conn. See TestS3IntFace.TestS3GetConn.
class TestS3CrtBucket(testcase.FDSTestCase):
    def __init__(self, parameters=None):
        super(TestS3CrtBucket, self).__init__(parameters)


    def runTest(self):
        self.test_passed = True

        if self.test_failure:
            self.log.warning("Skipping Case %s. stop-on-fail/failfast set " \
                             "and a previous test case has failed." %
                             self.__class__.__name__)
            return unittest.skip("stop-on-fail/failfast set and a previous " \
                                 "test case has failed.")
        else:
            self.log.info("Running Case %s." % self.__class__.__name__)

        try:
            if not self.test_S3CrtBucket():
                self.test_passed = False
        except Exception as inst:
            self.log.error("Creating an S3 bucket caused exception:")
            self.log.error(traceback.format_exc())
            self.log.error(inst.message)
            test_passed = False

        super(self.__class__, self).reportTestCaseResult(self.test_passed)

        # If there is any test fixture teardown to be done, do it here.

        self.assertTrue(self.test_passed)


    def test_S3CrtBucket(self):
        """
        Test Case:
        Attempt to create an S3 Bucket.
        """
        if not "s3" in self.parameters:
            self.log.error("No S3 interface object with which to create " \
                           "a bucket.")
            return False
        elif self.parameters["s3"].conn is None:
            self.log.error("No S3 connection with which to create a bucket.")
            return False
        else:
            self.log.info("Create an S3 bucket.")
            s3 = self.parameters["s3"]

            s3.bucket1 = s3.conn.create_bucket('fds_test_bucket_01')
            self.log.info(s3.bucket1)
            if not s3.bucket1:
                self.log.error("s3.conn.create_bucket() failed to create " \
                               "bucket bucket1.")
                return False

        return True


# This class contains the attributes and methods to test
# the FDS S3 interface to upload a small BLOB.
#
# You must have successfully created an S3 connection
# and stored it in self.parameters["s3"].conn (see TestS3IntFace.TestS3GetConn)
# and created a bucket and stored it in self.parameters["s3"].bucket1.
class TestS3LoadSBLOB(testcase.FDSTestCase):
    def __init__(self, parameters=None):
        super(TestS3LoadSBLOB, self).__init__(parameters)


    def runTest(self):
        self.test_passed = True

        if self.test_failure:
            self.log.warning("Skipping Case %s. stop-on-fail/failfast set " \
                             "and a previous test case has failed." %
                             self.__class__.__name__)
            return unittest.skip("stop-on-fail/failfast set and a previous " \
                                 "test case has failed.")
        else:
            self.log.info("Running Case %s." % self.__class__.__name__)

        try:
            if not self.test_S3LoadSBLOB():
                self.test_passed  = False
        except Exception as inst:
            self.log.error("Upload a 'small' (<= 2MiB) BLOB into an S3 " \
                           "bucket caused exception:")
            self.log.error(traceback.format_exc())
            self.log.error(inst.message)
            self.test_passed  = False

        super(self.__class__, self).reportTestCaseResult(self.test_passed)

        # If there is any test fixture teardown to be done, do it here.
        self.assertTrue(self.test_passed )


    def test_S3LoadSBLOB(self):
        """
        Test Case:
        Attempt to load a 'small' BLOB (<= 2MiB) into an S3 Bucket.
        """
        if (not "s3" in self.parameters) or (self.parameters["s3"].conn) \
            is None:
            self.log.error("No S3 connection with which to load a BLOB.")
            return False
        elif not self.parameters["s3"].bucket1:
            self.log.error("No S3 bucket with which to load a BLOB.")
            return False
        else:
            self.log.info("Load a 'small' BLOB (<= 2Mib) into an S3 bucket.")
            s3 = self.parameters["s3"]

            # Get file info
            source_path = config.SAMPLE_FILE
            source_size = os.stat(source_path).st_size
            self.log.info(s3.bucket1.name)
            # Get a Key/Value object for th bucket.
            k = Key(s3.bucket1)
            self.log.info(k)
            # Set the key.
            s3.keys.append("small")
            k.key = "small"

            self.log.info("Loading %s of size %d using Boto's " \
                          "Key.set_contents_from_string() interface." %
                          (source_path, source_size))
            self.log.info(source_path)
            self.log.info(source_size)
            # Set the value, write it to the bucket, and, while the file 
            # containing the value is still open, read it back to verify.
            with open(source_path, 'r')  as f:
                self.log.debug("Read from file %s:" % source_path)
                self.log.debug(f.read())
                k.set_contents_from_string(f.read())

                self.log.debug("Read from FDS %s:" % source_path)
                self.log.debug(k.get_contents_as_string())

                if k.get_contents_as_string() == (f.read()):
                    return True
                else:
                    self.log.error("File mis-match.")
                    return False


# This class contains the attributes and methods to test
# the FDS S3 interface to upload a "largish" BLOB in one piece.
#
# You must have successfully created an S3 connection
# and stored it in self.parameters["s3"].conn (see TestS3IntFace.TestS3GetConn)
# and created a bucket and stored it in self.parameters["s3"].bucket1.
class TestS3LoadFBLOB(testcase.FDSTestCase):
    def __init__(self, parameters=None):
        super(TestS3LoadFBLOB, self).__init__(parameters)


    def runTest(self):
        self.test_passed = True

        if self.test_failure:
            self.log.warning("Skipping Case %s. stop-on-fail/failfast set " \
                             "and a previous test case has failed." %
                             self.__class__.__name__)
            return unittest.skip("stop-on-fail/failfast set and a previous " \
                                 "test case has failed.")
        else:
            self.log.info("Running Case %s." % self.__class__.__name__)

        try:
            if not self.test_S3LoadFBLOB():
                self.test_passed = False
        except Exception as inst:
            self.log.error("Upload a 'largish' (<= 2MiB) BLOB into an S3 " \
                           "bucket in one piece caused exception:")
            self.log.error(traceback.format_exc())
            self.log.error(inst.message)
            test_passed = False

        super(self.__class__, self).reportTestCaseResult(self.test_passed)

        # If there is any test fixture teardown to be done, do it here.
        self.assertTrue(self.test_passed)


    def test_S3LoadFBLOB(self):
        """
        Test Case:
        Attempt to load a 'largish' BLOB(<=2MiB) into an S3 Bucket in one piece.
        """

        # Get the FdsConfigRun object for this test.
        if (not "s3" in self.parameters) or (self.parameters["s3"].conn) \
            is None:
            self.log.error("No S3 connection with which to load a BLOB.")
            return False
        elif not self.parameters["s3"].bucket1:
            self.log.error("No S3 bucket with which to load a BLOB.")
            return False
        else:
            self.log.info("Load a 'largish' BLOB (<= 2Mib) into an S3 bucket" \
                          "in one piece.")
            s3 = self.parameters["s3"]

            # Get file info
            source_path = config.SAMPLE_FILE
            source_size = os.stat(source_path).st_size

            # Get a Key/Value object for the bucket.
            k = Key(s3.bucket1)

            # Set the key.
            s3.keys.append("largish")
            k.key = "largish"

            self.log.info("Loading %s of size %d using Boto's " \
                          "Key.set_contents_from_filename() interface." %
                          (source_path, source_size))

            # Set the value and write it to the bucket.
            k.set_contents_from_filename(source_path)

            # Read it back to a file and then compare.
            dest_path = bin_dir + "/disk_type.py.boto"
            k.get_contents_to_filename(dest_path)

            test_passed = filecmp.cmp(source_path, dest_path, shallow=False)
            if not test_passed:
                self.log.error("File mis-match")

            os.remove(dest_path)

            return test_passed


# This class contains the attributes and methods to test
# the FDS S3 interface to upload a "largish" BLOB in one piece with meta-data.
#
# You must have successfully created an S3 connection
# and stored it in self.parameters["s3"].conn (see TestS3IntFace.TestS3GetConn)
# and created a bucket and stored it in self.parameters["s3"].bucket1.
class TestS3LoadMBLOB(testcase.FDSTestCase):
    def __init__(self, parameters=None):
        super(TestS3LoadMBLOB, self).__init__(parameters)


    def runTest(self):
        self.test_passed = True

        if self.test_failure:
            self.log.warning("Skipping Case %s. stop-on-fail/failfast set " \
                             "and a previous test case has failed." %
                             self.__class__.__name__)
            return unittest.skip("stop-on-fail/failfast set and a previous " \
                                 "test case has failed.")
        else:
            self.log.info("Running Case %s." % self.__class__.__name__)

        try:
            if not self.test_S3LoadMBLOB():
                self.test_passed = False
        except Exception as inst:
            self.log.error("Upload a 'largish' (<= 2MiB) BLOB into an S3 " \
                           "bucket in one piece with meta-data "
                            "caused exception:")
            self.log.error(traceback.format_exc())
            self.log.error(inst.message)
            test_passed = False

        super(self.__class__, self).reportTestCaseResult(self.test_passed)

        # If there is any test fixture teardown to be done, do it here.
        self.assertTrue(self.test_passed)


    def test_S3LoadMBLOB(self):
        """
        Test Case:
        Attempt to load a 'largish' BLOB (<= 2MiB) into an S3 Bucket
        in one piece with meta-data.
        """
        if (not "s3" in self.parameters) or (self.parameters["s3"].conn) \
            is None:
            self.log.error("No S3 connection with which to load a BLOB.")
            return False
        elif not self.parameters["s3"].bucket1:
            self.log.error("No S3 bucket with which to load a BLOB.")
            return False
        else:
            self.log.info("Load a 'largish' BLOB (<= 2Mib) into an S3 bucket" \
                          " in one piece with meta-data.")
            s3 = self.parameters["s3"]

            # Get file info
            source_path = bin_dir + "/disk_type.py"
            source_size = os.stat(source_path).st_size

            # Get a Key/Value object for the bucket.
            k = Key(s3.bucket1)

            # Set the key.
            s3.keys.append("largishWITHmetadata")
            k.key = "largishWITHmetadata"

            self.log.info("Loading %s of size %d using Boto's " \
                          "Key.set_contents_from_filename() interface "
                          "and setting meta-data." %
                          (source_path, source_size))

            # Set the value and write it to the bucket.
            k.set_metadata('meta1', 'This is the first metadata value')
            k.set_metadata('meta2', 'This is the second metadata value')
            k.set_contents_from_filename(source_path)

            # Read it back to a file and then compare.
            dest_path = bin_dir + "/disk_type.py.boto"
            k.get_contents_to_filename(dest_path)

            # Check the file.
            test_passed = filecmp.cmp(source_path, dest_path, shallow=False)

            # If the file looked OK, check the meta-data.
            if test_passed:
                meta = k.get_metadata('meta1')
                if meta != 'This is the first metadata value':
                    self.log.error("Meta-data 1 is incorrect: %s" % meta)
                    test_passed = False

                meta = k.get_metadata('meta2')
                if meta != 'This is the second metadata value':
                    self.log.error("Meta-data 2 is incorrect: %s" % meta)
                    test_passed = False
            else:
                self.log.error("File mis-match")

            os.remove(dest_path)

            return test_passed


# This class contains the attributes and methods to test
# the FDS S3 interface to upload a large BLOB.
#
# You must have successfully created an S3 connection
# and stored it in self.parameters["s3"].conn (see TestS3IntFace.TestS3GetConn)
# and created a bucket and stored it in self.parameters["s3"].bucket1.
class TestS3LoadLBLOB(testcase.FDSTestCase):
    def __init__(self, parameters=None):
        super(TestS3LoadLBLOB, self).__init__(parameters)


    def runTest(self):
        self.test_passed = True

        if self.test_failure:
            self.log.warning("Skipping Case %s. stop-on-fail/failfast set " \
                             "and a previous test case has failed." %
                             self.__class__.__name__)
            return unittest.skip("stop-on-fail/failfast set and a previous " \
                                 "test case has failed.")
        else:
            self.log.info("Running Case %s." % self.__class__.__name__)

        try:
            if not self.test_S3LoadLBLOB():
                self.test_passed = False
        except Exception as inst:
            self.log.error("Upload a 'large' (> 2MiB) BLOB into an S3 bucket" \
                           " caused exception:")
            self.log.error(traceback.format_exc())
            self.log.error(inst.message)
            test_passed = False

        super(self.__class__, self).reportTestCaseResult(self.test_passed)

        # If there is any test fixture teardown to be done, do it here.
        self.assertTrue(self.test_passed)


    def test_S3LoadLBLOB(self):
        """
        Test Case:
        Attempt to load a 'large BLOB (> 2MiB) into an S3 Bucket.
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]
        bin_dir = fdscfg.rt_env.get_bin_dir(debug=False)

        if (not "s3" in self.parameters) or (self.parameters["s3"].conn) \
            is None:
            self.log.error("No S3 connection with which to load a BLOB.")
            return False
        elif not self.parameters["s3"].bucket1:
            self.log.error("No S3 bucket with which to load a BLOB.")
            return False
        else:
            self.log.info("Load a 'large' BLOB (> 2Mib) into an S3 bucket.")
            s3 = self.parameters["s3"]

            # Get file info
            source_path = bin_dir + "/bare_am"
            source_size = os.stat(source_path).st_size

            # Create a multipart upload request
            s3.keys.append("large")
            mp = s3.bucket1.initiate_multipart_upload("large")

            # Use a chunk size of 50 MiB (feel free to change this)
            chunk_size = 52428800
            chunk_count = int(math.ceil(source_size / chunk_size))

            self.log.info("Loading %s of size %d using %d chunks of max " \
                          "size %d. using "
                          "Bobo's 'multi-part' upload interface" %
                          (source_path, source_size, chunk_count + 1,
                           chunk_size))

            # Send the file parts, using FileChunkIO to create a file-like 
            # object that points to a certain byte range within the original 
            # file. We set bytes to never exceed the original file size.
            for i in range(chunk_count + 1):
                self.log.info("Sending chunk %d." % (i + 1))
                offset = chunk_size * i
                bytes = min(chunk_size, source_size - offset)
                with FileChunkIO(source_path, 'r', offset=offset,
                         bytes=bytes) as fp:
                    mp.upload_part_from_file(fp, part_num=i + 1)

            # Finish the upload
            mp.complete_upload()

            # Read it back to a file and then compare.
            dest_path = bin_dir + "/bare_am.boto"

            k = Key(s3.bucket1)
            k.key = 'large'

            k.get_contents_to_filename(dest_path)

            test_passed = filecmp.cmp(source_path, dest_path, shallow=False)
            if not test_passed:
                self.log.error("File mis-match.")

            os.remove(dest_path)

            return test_passed


# This class contains the attributes and methods to test
# the FDS S3 interface to list the keys of a bucket.
#
# You must have successfully created an S3 connection
# and stored it in self.parameters["s3"].conn (see TestS3IntFace.TestS3GetConn)
# and created a bucket and stored it in self.parameters["s3"].bucket1.
class TestS3ListBucketKeys(testcase.FDSTestCase):
    def __init__(self, parameters=None):
        super(TestS3ListBucketKeys, self).__init__(parameters)


    def runTest(self):
        self.test_passed = True

        if self.test_failure:
            self.log.warning("Skipping Case %s. stop-on-fail/failfast set " \
                             "and a previous test case has failed." %
                             self.__class__.__name__)
            return unittest.skip("stop-on-fail/failfast set and a previous " \
                                 "test case has failed.")
        else:
            self.log.info("Running Case %s." % self.__class__.__name__)

        try:
            if not self.test_S3ListBucketKeys():
                self.test_passed = False
        except Exception as inst:
            self.log.error("Listing the keys of an S3 bucket caused exception:")
            self.log.error(traceback.format_exc())
            self.log.error(inst.message)
            self.test_passed = False

        super(self.__class__, self).reportTestCaseResult(self.test_passed)

        # If there is any test fixture teardown to be done, do it here.
        self.assertTrue(self.test_passed)


    def test_S3ListBucketKeys(self):
        """
        Test Case:
        Attempt to list the keys of an S3 Bucket.
        """

        if (not "s3" in self.parameters) or (self.parameters["s3"].conn) \
            is None:
            self.log.error("No S3 connection with which to load a BLOB.")
            return False
        elif not self.parameters["s3"].bucket1:
            self.log.error("No S3 bucket with which to load a BLOB.")
            return False
        else:
            self.log.info("List the keys of an S3 bucket.")
            test_passed = True

            s3 = self.parameters["s3"]

            cnt = 0
            for key in s3.bucket1.list():
                cnt += 1
                self.log.info(key.name)
                if s3.keys.count(key.name) != 1:
                    self.log.error("Unexpected key %s." % key.name)
                    test_passed = False

            if cnt != len(s3.keys):
                self.log.error("Missing key(s).")
                test_passed = False

            return test_passed


# This class contains the attributes and methods to test
# the FDS S3 interface to delete all the keys of a bucket.
#
# You must have successfully created an S3 connection
# and stored it in self.parameters["s3"].conn (see TestS3IntFace.TestS3GetConn)
# and created a bucket and stored it in self.parameters["s3"].bucket1.
class TestS3DelBucketKeys(testcase.FDSTestCase):
    def __init__(self, parameters=None):
        super(TestS3DelBucketKeys, self).__init__(parameters)


    def runTest(self):
        self.test_passed = True

        if self.test_failure:
            self.log.warning("Skipping Case %s. stop-on-fail/failfast set " \
                             "and a previous test case has failed." %
                             self.__class__.__name__)
            return unittest.skip("stop-on-fail/failfast set and a previous " \
                                 "test case has failed.")
        else:
            self.log.info("Running Case %s." % self.__class__.__name__)

        try:
            if not self.test_S3DelBucketKeys():
                self.test_passed = False
        except Exception as inst:
            self.log.error("Deleting all the keys of an S3 bucket caused " \
                           "exception:")
            self.log.error(traceback.format_exc())
            self.log.error(inst.message)
            self.test_passed = False

        super(self.__class__, self).reportTestCaseResult(self.test_passed)
        # If there is any test fixture teardown to be done, do it here.
        self.assertTrue(self.test_passed)


    def test_S3DelBucketKeys(self):
        """
        Test Case:
        Attempt to delete all the keys of an S3 Bucket.
        """

        if (not "s3" in self.parameters) or (self.parameters["s3"].conn) \
            is None:
            self.log.error("No S3 connection with which to load a BLOB.")
            return False
        elif not self.parameters["s3"].bucket1:
            self.log.error("No S3 bucket with which to load a BLOB.")
            return False
        else:
            self.log.info("Delete all the keys of an S3 bucket.")
            s3 = self.parameters["s3"]

            for key in s3.bucket1.list():
                key.delete()

            for key in s3.bucket1.list():
                self.log.error("Unexpected keys remaining in bucket: %s" %
                               key.name)
                return False
            return True


# This class contains the attributes and methods to test
# the FDS S3 interface to delete a bucket.
#
# You must have successfully created an S3 connection
# and stored it in self.parameters["s3"].conn (see TestS3IntFace.TestS3GetConn)
# and created a bucket and stored it in self.parameters["s3"].bucket1.
class TestS3DelBucket(testcase.FDSTestCase):
    def __init__(self, parameters=None):
        super(TestS3DelBucket, self).__init__(parameters)


    def runTest(self):
        self.test_passed = True

        if self.test_failure:
            self.log.warning("Skipping Case %s. stop-on-fail/failfast set" \
                             " and a previous test case has failed." %
                             self.name)
            return unittest.skip("stop-on-fail/failfast set and a previous" \
                                 " test case has failed.")
        else:
            self.log.info("Running Case %s." % self.name)

        try:
            if not self.test_S3DelBucket():
                self.test_passed = False
        except Exception as inst:
            self.log.error("Deleting an S3 bucket caused exception:")
            self.log.error(traceback.format_exc())
            self.log.error(inst.message)
            self.test_passed = False

        super(self.__class__, self).reportTestCaseResult(self.test_passed)

        # If there is any test fixture teardown to be done, do it here.
        self.assertTrue(self.test_passed)


    def test_S3DelBucket(self):
        """
        Test Case:
        Attempt to delete an S3 Bucket.
        """
        self.log.info(self.parameters["s3"].bucket1)
        if not ("s3" in self.parameters) or (self.parameters["s3"].conn) \
            is None:
            self.log.error("No S3 connection with which to load a BLOB.")
            return False
        elif not self.parameters["s3"].bucket1:
            self.log.error("No S3 bucket with which to load a BLOB.")
            return False
        else:
            self.log.info("Delete an S3 bucket.")
            s3 = self.parameters["s3"]
            s3.conn.delete_bucket('fds_test_bucket_01')
            if s3.conn.lookup('fds_test_bucket_01') != None:
                self.log.error("Unexpected bucket: fds_test_bucket_01")
                return False

            s3.bucket1 = None
            return True