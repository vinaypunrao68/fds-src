#!/usr/bin/python
#
# Copyright 2014 by Formation Data Systems, Inc.
#

# FDS test-case pattern requirements.
import unittest
import traceback
import TestCase

# Module-specific requirements
import sys
import os
import math
from filechunkio import FileChunkIO
import boto
from boto.s3 import connection

# Class to contain S3 objects used by these test cases.
class S3(object):
    def __init__(self, conn):
        self.conn = conn
        self.bucket1 = None


# This class contains the attributes and methods to test
# the FDS S3 interface to create a connection.
#
# You must have successfully retrieved an authorization token from
# OM and stored it in fdscfg.rt_om_node.auth_token. See TestOMIntFace.TestGetAuthToken.
class TestS3GetConn(TestCase.FDSTestCase):
    def __init__(self, parameters=None):
        super(TestS3GetConn, self).__init__(parameters)


    def runTest(self):
        test_passed = True

        if TestCase.pyUnitTCFailure:
            self.log.warning("Skipping Case %s. stop-on-fail/failfast set and a previous test case has failed." %
                             self.__class__.__name__)
            return unittest.skip("stop-on-fail/failfast set and a previous test case has failed.")
        else:
            self.log.info("Running Case %s." % self.__class__.__name__)

        try:
            if not self.test_S3GetConn():
                test_passed = False
        except Exception as inst:
            self.log.error("Getting an S3 connection caused exception:")
            self.log.error(traceback.format_exc())
            self.log.error(inst.message)
            test_passed = False

        super(self.__class__, self).reportTestCaseResult(test_passed)

        # If there is any test fixture teardown to be done, do it here.

        if self.parameters["pyUnit"]:
            self.assertTrue(test_passed)
        else:
            return test_passed


    def test_S3GetConn(self):
        """
        Test Case:
        Attempt to get an S3 connection.
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]
        om_node = fdscfg.rt_om_node

        if not hasattr(om_node, "auth_token"):
            self.log.error("No authorization token from OM. This is needed for an S3 connection.")
            return False
        else:
            self.log.info("Get an S3 connection on %s." %
                               om_node.nd_conf_dict['node-name'])

            s3conn = boto.connect_s3(aws_access_key_id='admin',
                                            aws_secret_access_key=om_node.auth_token,
                                            host=om_node.nd_conf_dict['ip'],
                                            port=8443,
                                            calling_format=boto.s3.connection.OrdinaryCallingFormat())

            if not s3conn:
                self.log.error("boto.connect_s3() on %s did not return an S3 connection." %
                               om_node.nd_conf_dict['node-name'])
                return False

            self.parameters["s3"] = S3(s3conn)

        return True


# This class contains the attributes and methods to test
# the FDS S3 interface to create a bucket.
#
# You must have successfully created an S3 connection
# and stored it in self.parameters["s3"].conn. See TestS3IntFace.TestS3GetConn.
class TestS3CrtBucket(TestCase.FDSTestCase):
    def __init__(self, parameters=None):
        super(TestS3CrtBucket, self).__init__(parameters)


    def runTest(self):
        test_passed = True

        if TestCase.pyUnitTCFailure:
            self.log.warning("Skipping Case %s. stop-on-fail/failfast set and a previous test case has failed." %
                             self.__class__.__name__)
            return unittest.skip("stop-on-fail/failfast set and a previous test case has failed.")
        else:
            self.log.info("Running Case %s." % self.__class__.__name__)

        try:
            if not self.test_S3CrtBucket():
                test_passed = False
        except Exception as inst:
            self.log.error("Creating an S3 bucket caused exception:")
            self.log.error(traceback.format_exc())
            self.log.error(inst.message)
            test_passed = False

        super(self.__class__, self).reportTestCaseResult(test_passed)

        # If there is any test fixture teardown to be done, do it here.

        if self.parameters["pyUnit"]:
            self.assertTrue(test_passed)
        else:
            return test_passed


    def test_S3CrtBucket(self):
        """
        Test Case:
        Attempt to create an S3 Bucket.
        """

        if not "s3" in self.parameters:
            self.log.error("No S3 interface object with which to create a bucket.")
            return False
        elif self.parameters["s3"].conn is None:
            self.log.error("No S3 connection with which to create a bucket.")
            return False
        else:
            self.log.info("Create an S3 bucket.")
            s3 = self.parameters["s3"]

            s3.bucket1 = s3.conn.create_bucket('bucket1')

            if not s3.bucket1:
                self.log.error("s3.conn.create_bucket() failed to create bucket bucket1.")
                return False

        return True


# This class contains the attributes and methods to test
# the FDS S3 interface to upload a large BLOB.
#
# You must have successfully created an S3 connection
# and stored it in self.parameters["s3"].conn (see TestS3IntFace.TestS3GetConn)
# and created a bucket and stored it in self.parameters["s3"].bucket1.
class TestS3LoadBLOB(TestCase.FDSTestCase):
    def __init__(self, parameters=None):
        super(TestS3LoadBLOB, self).__init__(parameters)


    def runTest(self):
        test_passed = True

        if TestCase.pyUnitTCFailure:
            self.log.warning("Skipping Case %s. stop-on-fail/failfast set and a previous test case has failed." %
                             self.__class__.__name__)
            return unittest.skip("stop-on-fail/failfast set and a previous test case has failed.")
        else:
            self.log.info("Running Case %s." % self.__class__.__name__)

        try:
            if not self.test_S3LoadBLOB():
                test_passed = False
        except Exception as inst:
            self.log.error("Upload a 'large' (> 2MiB) BLOB into an S3 bucket caused exception:")
            self.log.error(traceback.format_exc())
            self.log.error(inst.message)
            test_passed = False

        super(self.__class__, self).reportTestCaseResult(test_passed)

        # If there is any test fixture teardown to be done, do it here.

        if self.parameters["pyUnit"]:
            self.assertTrue(test_passed)
        else:
            return test_passed


    def test_S3LoadBLOB(self):
        """
        Test Case:
        Attempt to load a 'large BLOB (> 2MiB) into an S3 Bucket.
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]
        bin_dir = fdscfg.rt_env.get_bin_dir(debug=False)

        if (not "s3" in self.parameters) or (self.parameters["s3"].conn) is None:
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
            mp = s3.bucket1.initiate_multipart_upload(os.path.basename(source_path))

            # Use a chunk size of 50 MiB (feel free to change this)
            chunk_size = 52428800
            chunk_count = int(math.ceil(source_size / chunk_size))

            self.log.info("Loading %s of size %d using %d chunks of max size %d." %
                          (source_path, source_size, chunk_count + 1, chunk_size))

            # Send the file parts, using FileChunkIO to create a file-like object
            # that points to a certain byte range within the original file. We
            # set bytes to never exceed the original file size.
            for i in range(chunk_count + 1):
                self.log.info("Sending chunk %d." % (i + 1))
                offset = chunk_size * i
                bytes = min(chunk_size, source_size - offset)
                with FileChunkIO(source_path, 'r', offset=offset,
                         bytes=bytes) as fp:
                    mp.upload_part_from_file(fp, part_num=i + 1)

            # Finish the upload
            mp.complete_upload()

        return True


# This class contains the attributes and methods to test
# the FDS S3 interface to list the keys of a bucket.
#
# You must have successfully created an S3 connection
# and stored it in self.parameters["s3"].conn (see TestS3IntFace.TestS3GetConn)
# and created a bucket and stored it in self.parameters["s3"].bucket1.
class TestS3ListBucketKeys(TestCase.FDSTestCase):
    def __init__(self, parameters=None):
        super(TestS3ListBucketKeys, self).__init__(parameters)


    def runTest(self):
        test_passed = True

        if TestCase.pyUnitTCFailure:
            self.log.warning("Skipping Case %s. stop-on-fail/failfast set and a previous test case has failed." %
                             self.__class__.__name__)
            return unittest.skip("stop-on-fail/failfast set and a previous test case has failed.")
        else:
            self.log.info("Running Case %s." % self.__class__.__name__)

        try:
            if not self.test_S3ListBucketKeys():
                test_passed = False
        except Exception as inst:
            self.log.error("Listing the keys of an S3 bucket caused exception:")
            self.log.error(traceback.format_exc())
            self.log.error(inst.message)
            test_passed = False

        super(self.__class__, self).reportTestCaseResult(test_passed)

        # If there is any test fixture teardown to be done, do it here.

        if self.parameters["pyUnit"]:
            self.assertTrue(test_passed)
        else:
            return test_passed


    def test_S3ListBucketKeys(self):
        """
        Test Case:
        Attempt to list the keys of an S3 Bucket.
        """

        if (not "s3" in self.parameters) or (self.parameters["s3"].conn) is None:
            self.log.error("No S3 connection with which to load a BLOB.")
            return False
        elif not self.parameters["s3"].bucket1:
            self.log.error("No S3 bucket with which to load a BLOB.")
            return False
        else:
            self.log.info("List the keys of an S3 bucket.")
            s3 = self.parameters["s3"]

            for key in s3.bucket1.list():
                self.log.info(key, key.storage_class)

        return True


# This class contains the attributes and methods to test
# the FDS S3 interface to delete all the keys of a bucket.
#
# You must have successfully created an S3 connection
# and stored it in self.parameters["s3"].conn (see TestS3IntFace.TestS3GetConn)
# and created a bucket and stored it in self.parameters["s3"].bucket1.
class TestS3DelBucketKeys(TestCase.FDSTestCase):
    def __init__(self, parameters=None):
        super(TestS3DelBucketKeys, self).__init__(parameters)


    def runTest(self):
        test_passed = True

        if TestCase.pyUnitTCFailure:
            self.log.warning("Skipping Case %s. stop-on-fail/failfast set and a previous test case has failed." %
                             self.__class__.__name__)
            return unittest.skip("stop-on-fail/failfast set and a previous test case has failed.")
        else:
            self.log.info("Running Case %s." % self.__class__.__name__)

        try:
            if not self.test_S3DelBucketKeys():
                test_passed = False
        except Exception as inst:
            self.log.error("Deleting all the keys of an S3 bucket caused exception:")
            self.log.error(traceback.format_exc())
            self.log.error(inst.message)
            test_passed = False

        super(self.__class__, self).reportTestCaseResult(test_passed)

        # If there is any test fixture teardown to be done, do it here.

        if self.parameters["pyUnit"]:
            self.assertTrue(test_passed)
        else:
            return test_passed


    def test_S3DelBucketKeys(self):
        """
        Test Case:
        Attempt to delete all the keys of an S3 Bucket.
        """

        if (not "s3" in self.parameters) or (self.parameters["s3"].conn) is None:
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

        return True


# This class contains the attributes and methods to test
# the FDS S3 interface to delete a bucket.
#
# You must have successfully created an S3 connection
# and stored it in self.parameters["s3"].conn (see TestS3IntFace.TestS3GetConn)
# and created a bucket and stored it in self.parameters["s3"].bucket1.
class TestS3DelBucket(TestCase.FDSTestCase):
    def __init__(self, parameters=None):
        super(TestS3DelBucket, self).__init__(parameters)


    def runTest(self):
        test_passed = True

        if TestCase.pyUnitTCFailure:
            self.log.warning("Skipping Case %s. stop-on-fail/failfast set and a previous test case has failed." %
                             self.__class__.__name__)
            return unittest.skip("stop-on-fail/failfast set and a previous test case has failed.")
        else:
            self.log.info("Running Case %s." % self.__class__.__name__)

        try:
            if not self.test_S3DelBucket():
                test_passed = False
        except Exception as inst:
            self.log.error("Deleting an S3 bucket caused exception:")
            self.log.error(traceback.format_exc())
            self.log.error(inst.message)
            test_passed = False

        super(self.__class__, self).reportTestCaseResult(test_passed)

        # If there is any test fixture teardown to be done, do it here.

        if self.parameters["pyUnit"]:
            self.assertTrue(test_passed)
        else:
            return test_passed


    def test_S3DelBucket(self):
        """
        Test Case:
        Attempt to delete an S3 Bucket.
        """

        if not ("s3" in self.parameters) or (self.parameters["s3"].conn) is None:
            self.log.error("No S3 connection with which to load a BLOB.")
            return False
        elif not self.parameters["s3"].bucket1:
            self.log.error("No S3 bucket with which to load a BLOB.")
            return False
        else:
            self.log.info("Delete an S3 bucket.")
            s3 = self.parameters["s3"]

            s3.conn.delete_bucket('bucket1')
            s3.bucket1 = None

        return True


if __name__ == '__main__':
    TestCase.FDSTestCase.fdsGetCmdLineConfigs(sys.argv)
    unittest.main()