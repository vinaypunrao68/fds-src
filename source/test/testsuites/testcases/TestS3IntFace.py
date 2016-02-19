#!/usr/bin/python
#
# Copyright 2014 by Formation Data Systems, Inc.
#

# FDS test-case pattern requirements.
import unittest
import TestCase

# Module-specific requirements
import sys
import uuid
import os
import math
import hashlib
import httplib
from filechunkio import FileChunkIO
import boto
from boto.s3 import connection
from boto.s3.key import Key
import filecmp
import random
import types
import string
import re
from threading import Timer
import logging
from fdslib.TestUtils import get_resource

class Helper:
    @staticmethod
    def tobytes(num):
        if type(num) == types.IntType:
            return num

        if type(num) == types.StringType:
            if num.isdigit(): return int(num)
            m = re.search(r'[A-Za-z]', num)
            if m.start() <=0:
                return num

            n = int(num[0:m.start()])
            c = num[m.start()].upper()
            if c == 'K': return n*1024;
            if c == 'M': return n*1024*1024;
            if c == 'G': return n*1024*1024*1024;

    @staticmethod
    def boolean(value):
        if type(value) == types.StringType:
            value = value.lower()
        return value in ['true', '1', 'yes', 1, 'ok', 'set', True]

    @staticmethod
    def genData(length=10, seed=None):
        r = random.Random(seed)
        return ''.join([r.choice(string.ascii_lowercase) for i in range(0, Helper.tobytes(length))])

    @staticmethod
    def bucketName(seed=None):
        return 'volume-{}'.format( Helper.genData(10, seed))

    @staticmethod
    def keyName(seed=None):
        return 'key-{}'.format( Helper.genData(12, seed))

    @staticmethod
    def checkS3Info(self, bucket=None):
        if (not "s3" in self.parameters) or (self.parameters["s3"].conn) is None:
            self.log.error("No S3 connection")
            return False

        if bucket is not None:
            self.parameters["s3"].bucket1 = self.parameters["s3"].conn.lookup(bucket)

        if not self.parameters["s3"].bucket1:
            self.log.error("No S3 bucket info for {}.".format(bucket))
            return False

        return True

# Class to contain S3 objects used by these test cases.
class S3(object):
    def __init__(self, conn):
        self.conn = conn
        self.bucket1 = None
        self.keys = []  # As we add keys to the bucket, add them here as well.
        self.verifiers = {} # As we track objects to verify, add object name
                            # and hash here


# This class contains the attributes and methods to test
# the FDS S3 interface to create a connection.
#
# You must have successfully retrieved an authorization token from
# OM and stored it in fdscfg.rt_om_node.auth_token. See TestOMIntFace.TestGetAuthToken.
class TestS3GetConn(TestCase.FDSTestCase):
    def __init__(self, parameters=None):
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_S3GetConn,
                                             "Getting an S3 connection")

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
            s3conn = None
            retryCount = 0
            maxRetries = 20
            backoff_factor = 0.5
            while retryCount < maxRetries:
              s3conn = boto.connect_s3(aws_access_key_id='admin',
                                       aws_secret_access_key=om_node.auth_token,
                                       host=om_node.nd_conf_dict['ip'],
                                       port=8443,
                                       calling_format=boto.s3.connection.OrdinaryCallingFormat())
              if not s3conn:
                 retryCount += 1
                 self.log.warn( "boto.connect_s3() on %s still isn't available, retry %d." % (om_node.nd_conf_dict['node-name'], count))
                 if retryCount < maxRetries:
                   retryTime = 1 + ( (retryCount - 1) * backoff_factor )
                   time.sleep(retryTime)
              else:
                break

            if not s3conn:
                self.log.error("boto.connect_s3() on %s did not return an S3 connection." %
                               om_node.nd_conf_dict['node-name'])
                return False

            # This method may be called multiple times in the even the connection
            # is closed and needs to be re-opened. But we only want to instantiate the
            # S3 object once.
            if not "s3" in self.parameters:
                self.parameters["s3"] = S3(s3conn)
            else:
                self.parameters["s3"].conn = s3conn

        return True


# This class contains the attributes and methods to test
# the FDS S3 interface to close a connection.
#
# You must have successfully created a connection.
class TestS3CloseConn(TestCase.FDSTestCase):
    def __init__(self, parameters=None):
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_S3CloseConn,
                                             "Closing an S3 connection")

    def test_S3CloseConn(self):
        """
        Test Case:
        Attempt to close an S3 connection.
        """

        if not "s3" in self.parameters:
            self.log.error("No S3 interface object so no connection to close.")
            return False
        elif self.parameters["s3"].conn is None:
            self.log.error("No S3 connection to close.")
            return False
        else:
            self.log.info("Close an S3 connection.")
            s3 = self.parameters["s3"]
            try:
                s3.conn.close()
            except Exception as e:
                self.log.error("Could not close S3 connection.")
                self.log.error(e.message)
                return False

        return True


# This class contains the attributes and methods to test
# the FDS S3 interface to create a bucket.
#
# You must have successfully created an S3 connection
# and stored it in self.parameters["s3"].conn. See TestS3IntFace.TestS3GetConn.
class TestS3CrtBucket(TestCase.FDSTestCase):
    def __init__(self, parameters=None, bucket='bucket1'):
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_S3CrtBucket,
                                             "Creating an S3 bucket")
        self.passedBucket = bucket
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

        self.log.info("Create an S3 bucket : [{}]".format(self.passedBucket))
        s3 = self.parameters["s3"]

        s3.bucket1 = s3.conn.create_bucket(self.passedBucket)

        if not s3.bucket1:
            self.log.error("s3.conn.create_bucket() failed to create bucket bucket1.")
            return False

        return True


class TestS3Acls(TestCase.FDSTestCase):
    def __init__(self, parameters=None):
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_S3Acls,
                                             "Testing ACLs with Boto")

    def test_S3Acls(self):
        """
        Test Case:
        Attempt to set ACL parameters with Boto.
        """

        if not "s3" in self.parameters:
            self.log.error("No S3 interface object with which to create a bucket.")
            return False
        elif self.parameters["s3"].conn is None:
            self.log.error("No S3 connection with which to create a bucket.")
            return False
        else:
            s3 = self.parameters["s3"]
            bucket_name = str(uuid.uuid4())
            random_bucket = s3.conn.create_bucket(bucket_name)
            key_name = str(uuid.uuid4())
            conn = httplib.HTTPConnection("localhost:8000")

            # Try a PUT without setting the ACL first, should be rejected
            conn.request("PUT", "/" + bucket_name + "/" + key_name, "file contents")
            result = conn.getresponse()
            self.assertEqual(403, result.status)

            # Anonymous PUT should be accepted after ACL has been set
            random_bucket.set_acl('public-read-write')
            conn = httplib.HTTPConnection("localhost:8000")
            conn.request("PUT", "/" + bucket_name + "/" + key_name, "file contents")
            result = conn.getresponse()
            self.assertEqual(200, result.status)


        return True


# This class contains the attributes and methods to test
# the FDS S3 interface to upload a zero-length BLOB.
#
# You must have successfully created an S3 connection
# and stored it in self.parameters["s3"].conn (see TestS3IntFace.TestS3GetConn)
# and created a bucket and stored it in self.parameters["s3"].bucket1.
class TestS3LoadZBLOB(TestCase.FDSTestCase):
    def __init__(self, parameters=None):
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_S3LoadZBLOB,
                                             "Upload a zero-length BLOB into an S3 bucket")

    def test_S3LoadZBLOB(self):
        """
        Test Case:
        Attempt to load a zero-length BLOB into an S3 Bucket.
        """

        if (not "s3" in self.parameters) or (self.parameters["s3"].conn) is None:
            self.log.error("No S3 connection with which to load a BLOB.")
            return False
        elif not self.parameters["s3"].bucket1:
            self.log.error("No S3 bucket with which to load a BLOB.")
            return False
        else:
            self.log.info("Load a zero-length BLOB into an S3 bucket.")
            s3 = self.parameters["s3"]

            # Get file info
            source_path = "/dev/null"
            source_size = os.stat(source_path).st_size

            # Get a Key/Value object for th bucket.
            k = Key(s3.bucket1)

            # Set the key.
            s3.keys.append("zero")
            k.key = "zero"

            self.log.info("Loading %s of size %d using Boto's Key.set_contents_from_string() interface." %
                          (source_path, source_size))

            # Set the value, write it to the bucket, and, while the file containing the value is still
            # open, read it back to verify.
            with open(source_path, 'r')  as f:
                f.seek(0)
                k.set_contents_from_string(f.read())

                f.seek(0)
                if k.get_contents_as_string() == (f.read()):
                    return True
                else:
                    self.log.error("File mis-match.")
                    return False


# This class contains the attributes and methods to test
# the FDS S3 interface to upload a small BLOB.
#
# You must have successfully created an S3 connection
# and stored it in self.parameters["s3"].conn (see TestS3IntFace.TestS3GetConn)
# and created a bucket and stored it in self.parameters["s3"].bucket1.
class TestS3LoadSBLOB(TestCase.FDSTestCase):
    def __init__(self, parameters=None):
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_S3LoadSBLOB,
                                             "Upload a 'small' (<= 2MiB) BLOB into an S3 bucket")

    def test_S3LoadSBLOB(self):
        """
        Test Case:
        Attempt to load a 'small' BLOB (<= 2MiB) into an S3 Bucket.
        """

        bin_dir = '../../Build/linux-x86_64.debug/bin'

        if (not "s3" in self.parameters) or (self.parameters["s3"].conn) is None:
            self.log.error("No S3 connection with which to load a BLOB.")
            return False
        elif not self.parameters["s3"].bucket1:
            self.log.error("No S3 bucket with which to load a BLOB.")
            return False
        else:
            self.log.info("Load a 'small' BLOB (<= 2Mib) into an S3 bucket.")
            s3 = self.parameters["s3"]

            # Get file info
            source_path = bin_dir + "/orchMgr"
            source_size = os.stat(source_path).st_size

            # Get a Key/Value object for th bucket.
            k = Key(s3.bucket1)

            # Set the key.
            s3.keys.append("small")
            k.key = "small"

            self.log.info("Loading %s of size %d using Boto's Key.set_contents_from_string() interface." %
                          (source_path, source_size))

            # Set the value, write it to the bucket, and, while the file containing the value is still
            # open, read it back to verify.
            with open(source_path, 'r')  as f:
                self.log.debug("Read from file %s:" % source_path)

                f.seek(0)
                self.log.debug(f.read())

                f.seek(0)
                k.set_contents_from_string(f.read())

                self.log.debug("Read from FDS %s:" % source_path)
                self.log.debug(k.get_contents_as_string())

                f.seek(0)
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
#
# @param bucket If provided, overrides whatever is stored in self.parameters["s3"].bucket1
# @param verify If "true" verifies the blob loaded against the file from which it was loaded.
# @param inputfile If provided, overrides the default input file. If the input file is actually
#                  located in the SysTest resources directory, you may indicate that by using the
#                  value "RESOURCES/<inputfile.name>".
# @param key If provided this will be the object key of the loaded Blob. Otherwise a default is used.
class TestS3LoadFBLOB(TestCase.FDSTestCase):
    def __init__(self, parameters=None, bucket=None, verify="true", inputfile=None, key=None):
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_S3LoadFBLOB,
                                             "Upload a 'largish' (<= 2MiB) BLOB into an S3 bucket in one piece")

        self.passedBucket = bucket

        if verify == "true":
            self.passedVerify = True
        else:
            self.passedVerify = False

        self.passedInputFile = inputfile
        self.passedKey = key

    def test_S3LoadFBLOB(self):
        """
        Test Case:
        Attempt to load a 'largish' BLOB (<= 2MiB and therefore may not cross data object boundaries) into an S3 Bucket in one piece.
        """

        # TODO: Need to *not* hard-code this.
        bin_dir = '../../Build/linux-x86_64.debug/bin'

        # Make sure we're good to go with S3 and our bucket.
        if not Helper.checkS3Info(self, self.passedBucket):
            return False

        s3 = self.parameters["s3"]

        # Get file to be loaded.
        source_path = ""
        if self.passedInputFile is None:
            source_path = bin_dir + "/disk_type.py"  # Our default will not cross object boundaries.
        else:
            # If our HLQ is "RESOURCES", then we'll pull the file from the SysTest framework's resources repository.
            if os.path.dirname(self.passedInputFile) == "RESOURCES":
                source_path = get_resource(self, os.path.basename(self.passedInputFile))
            else:
                source_path = self.passedInputFile

        source_size = os.stat(source_path).st_size

        # Get a Key/Value object for the bucket.
        k = Key(s3.bucket1)

        # Set the key.
        lkey = "largish"
        if self.passedKey is not None:
            lkey = self.passedKey

        s3.keys.append(lkey)
        k.key = lkey

        self.log.info("Loading %s of size %d bytes with key %s with "
                      "Boto's Key.set_contents_from_filename() interface." %
                      (source_path, source_size, lkey))

        # Set the value and write it to the bucket.
        k.set_contents_from_filename(source_path)

        test_passed = True
        if self.passedVerify:
            # Read it back to a file and then compare.
            dest_path = bin_dir + "/TestS3LoadFBLOB-loadedfile.dump"

            k.get_contents_to_filename(dest_path)

            test_passed = filecmp.cmp(source_path, dest_path, shallow=False)
            if not test_passed:
                self.log.error("File mis-match")

            os.remove(dest_path)

        return test_passed


# This class contains the attributes and methods to test
# that the largish BLOB is in tact.
#
# You must have successfully created an S3 connection
# and stored it in self.parameters["s3"].conn (see TestS3IntFace.TestS3GetConn)
# and created a bucket and stored it in self.parameters["s3"].bucket1.
#
# You must also have successfully executed test case TestS3LoadFBLOB,
class TestS3VerifyFBLOB(TestCase.FDSTestCase):
    def __init__(self, parameters=None, bucket=None, key=None, comparefile=None):
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_S3VerifyFBLOB,
                                             "Verify the 'largish' (<= 2MiB) BLOB")
        self.passedBucket = bucket
        self.passedKey = key
        self.passedCompareFile = comparefile

    def test_S3VerifyFBLOB(self):
        """
        Test Case:
        Attempt to verify the 'largish' BLOB (<= 2MiB).
        """

        # Get the FdsConfigRun object for this test.
        bin_dir = '../../Build/linux-x86_64.debug/bin'

        # Make sure we're good to go with S3 and our bucket.
        if not Helper.checkS3Info(self, self.passedBucket):
            return False

        self.log.info("Verify the 'largish' BLOB (<= 2Mib).")

        s3 = self.parameters["s3"]

        # Get file to be checked.
        compare_path = ""
        if self.passedCompareFile is None:
            compare_path = bin_dir + "/disk_type.py"  # Our default will not cross object boundaries.
        else:
            # If our HLQ is "RESOURCES", then we'll pull the file from the SysTest framework's resources repository.
            if os.path.dirname(self.passedCompareFile) == "RESOURCES":
                compare_path = get_resource(self, os.path.basename(self.passedCompareFile))
            else:
                compare_path = self.passedCompareFile

        compare_size = os.stat(compare_path).st_size

        # Get a Key/Value object for the bucket.
        k = Key(s3.bucket1)

        # Set the key.
        lkey = "largish"
        if self.passedKey is not None:
            lkey = self.passedKey

        s3.keys.append(lkey)
        k.key = lkey

        self.log.info("Comparing %s of size %d bytes with key %s with "
                      "Boto's Key.get_contents_to_filename() interface." %
                      (compare_path, compare_size, lkey))

        # Read it back to a file and then compare.
        dest_path = bin_dir + "/TestS3VerifyFBLOB-loadedfile.dump"

        k.get_contents_to_filename(dest_path)

        test_passed = filecmp.cmp(compare_path, dest_path, shallow=False)
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
class TestS3LoadMBLOB(TestCase.FDSTestCase):
    def __init__(self, parameters=None, bucket=None):
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_S3LoadMBLOB,
                                             "Upload a 'largish' (<= 2MiB) BLOB into an S3 bucket in one piece with meta-data")

        self.passedBucket = bucket

    def test_S3LoadMBLOB(self):
        """
        Test Case:
        Attempt to load a 'largish' BLOB (<= 2MiB) into an S3 Bucket in one piece with meta-data.
        """

        bin_dir = '../../Build/linux-x86_64.debug/bin'

        # Make sure we're good to go with S3 and our bucket.
        if not Helper.checkS3Info(self, self.passedBucket):
            return False

        self.log.info("Load a 'largish' BLOB (<= 2Mib) into an S3 bucket in one piece with meta-data.")
        s3 = self.parameters["s3"]

        # Get file info
        source_path = bin_dir + "/disk_type.py"
        source_size = os.stat(source_path).st_size

        # Get a Key/Value object for the bucket.
        k = Key(s3.bucket1)

        # Set the key.
        s3.keys.append("largishWITHmetadata")
        k.key = "largishWITHmetadata"

        self.log.info("Loading %s of size %d using Boto's Key.set_contents_from_filename() interface "
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
# that the largish BLOB with meta-data is in tact.
#
# You must have successfully created an S3 connection
# and stored it in self.parameters["s3"].conn (see TestS3IntFace.TestS3GetConn)
# and created a bucket and stored it in self.parameters["s3"].bucket1.
#
# You must also have sucessfully executed test case TestS3LoadMBLOB,
class TestS3VerifyMBLOB(TestCase.FDSTestCase):
    def __init__(self, parameters=None):
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_S3VerifyMBLOB,
                                             "Verify the 'largish' (<= 2MiB) BLOB with meta-data")

    def test_S3VerifyMBLOB(self):
        """
        Test Case:
        Attempt to verify the 'largish' BLOB (<= 2MiB) with meta-data.
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]
        bin_dir = '../../Build/linux-x86_64.debug/bin'
        if (not "s3" in self.parameters) or (self.parameters["s3"].conn) is None:
            self.log.error("No S3 connection with which to load a BLOB.")
            return False
        elif not self.parameters["s3"].bucket1:
            self.log.error("No S3 bucket with which to load a BLOB.")
            return False
        else:
            self.log.info("Verify the 'largish' BLOB (<= 2Mib) with meta-data.")
            s3 = self.parameters["s3"]

            # Get file info
            source_path = bin_dir + "/disk_type.py"
            source_size = os.stat(source_path).st_size

            # Get a Key/Value object for the bucket.
            k = Key(s3.bucket1)

            # Set the key.
            s3.keys.append("largishWITHmetadata")
            k.key = "largishWITHmetadata"

            # Read the BLOB to a file and then compare.
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
#
# @param bucket If provided, overrides whatever is stored in self.parameters["s3"].bucket1
# @param verify If "true" verifies the blob loaded against the file from which it was loaded.
# @param inputfile If provided, overrides the default input file. If the input file is actually
#                  located in the SysTest resources directory, you may indicate that by using the
#                  value "RESOURCES/<inputfile.name>".
# @param key If provided this will be the object key of the loaded Blob. Otherwise a default is used.
class TestS3LoadLBLOB(TestCase.FDSTestCase):
    def __init__(self, parameters=None, bucket=None, verify="true", inputfile=None, key=None):
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_S3LoadLBLOB,
                                             "Upload a 'large' (> 2MiB) BLOB into an S3 bucket")

        self.passedBucket = bucket

        if verify == "true":
            self.passedVerify = True
        else:
            self.passedVerify = False

        self.passedInputFile = inputfile
        self.passedKey = key

    def test_S3LoadLBLOB(self):
        """
        Test Case:
        Attempt to load a file into an S3 Bucket by parts. By default we would like this file to
        be large enough to cross data object boundaries.
        """

        # TODO: Need to *not* hard-code this.
        bin_dir = '../../Build/linux-x86_64.debug/bin'

        # Make sure we're good to go with S3 and our bucket.
        if not Helper.checkS3Info(self, self.passedBucket):
            return False

        s3 = self.parameters["s3"]

        # Get file to be loaded.
        source_path = ""
        if self.passedInputFile is None:
            source_path = bin_dir + "/StorMgr"  # We want our default to be big enough to cross object boundaries.
        else:
            # If our HLQ is "RESOURCES", then we'll pull the file from the SysTest framework's resources repository.
            if os.path.dirname(self.passedInputFile) == "RESOURCES":
                source_path = get_resource(self, os.path.basename(self.passedInputFile))
            else:
                source_path = self.passedInputFile

        source_size = os.stat(source_path).st_size

        # Create a multipart upload request
        lkey = "large"
        if self.passedKey is not None:
            lkey = self.passedKey

        s3.keys.append(lkey)
        mp = s3.bucket1.initiate_multipart_upload(lkey)

        # Use a chunk size of 50 MiB (feel free to change this)
        chunk_size = 52428800
        chunk_count = int(math.ceil(float(source_size) / chunk_size))
        md5sum = str(hashlib.md5(open(source_path, 'rb').read()).hexdigest())

        self.log.info("Loading %s of size %d bytes [md5: %s] with key %s using %d chunks of max size %d with "
                      "Boto's 'multi-part' upload interface." %
                      (source_path, source_size, md5sum, lkey, chunk_count, chunk_size))

        # Send the file parts, using FileChunkIO to create a file-like object
        # that points to a certain byte range within the original file. We
        # set bytes to never exceed the original file size.
        for i in range(chunk_count):
            self.log.info("Sending chunk %d." % (i + 1))
            offset = chunk_size * i
            bytes = min(chunk_size, source_size - offset)
            with FileChunkIO(source_path, 'r', offset=offset,
                     bytes=bytes) as fp:
                mp.upload_part_from_file(fp, part_num=i + 1)

        # Finish the upload
        completed_upload = mp.complete_upload()
        if not md5sum != completed_upload.etag:
            self.log.info("Etag does not match md5 sum of file: [ " + md5sum + " ] != [ " + completed_upload.etag + "]")
            return False
        else:
            self.log.info("Etag matched: [" + completed_upload.etag + "]")

        test_passed = True
        if self.passedVerify:
            # Read it back to a file and then compare.
            dest_path = bin_dir + "/TestS3LoadLBLOB-loadedfile.dump"

            k = Key(s3.bucket1)
            k.key = lkey

            k.get_contents_to_filename(filename=dest_path)

            test_passed = filecmp.cmp(source_path, dest_path, shallow=False)
            if not test_passed:
                md5sum = str(hashlib.md5(open(source_path, 'rb').read()).hexdigest())
                self.log.error("File mis-match. [" + md5sum + "]")

            os.remove(dest_path)

        return test_passed


# This class contains the attributes and methods to test
# the FDS S3 interface to verify a large BLOB.
#
# You must have successfully created an S3 connection
# and stored it in self.parameters["s3"].conn (see TestS3IntFace.TestS3GetConn)
# and created a bucket and stored it in self.parameters["s3"].bucket1.
#
# @param bucket If provided, overrides whatever is stored in self.parameters["s3"].bucket1
# @param verify If "true" verifies the blob loaded against the file from which it was loaded.
# @param inputfile If provided, overrides the default input file. If the input file is actually
#                  located in the SysTest resources directory, you may indicate that by using the
#                  value "RESOURCES/<inputfile.name>".
# @param key If provided this will be the object key of the loaded Blob. Otherwise a default is used.
class TestS3VerifyLBLOB(TestCase.FDSTestCase):
    def __init__(self, parameters=None, bucket=None, key=None, comparefile=None):
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_S3VerifyLBLOB,
                                             "Verify a 'large' (> 2MiB) BLOB in an S3 bucket")

        self.passedBucket = bucket
        self.passedKey = key
        self.passedCompareFile = comparefile

    def test_S3VerifyLBLOB(self):
        """
        Test Case:
        Attempt to verify a file in an S3 Bucket loaded by parts. By default we would like this file to
        be large enough to cross data object boundaries.
        """

        # TODO: Need to *not* hard-code this.
        bin_dir = '../../Build/linux-x86_64.debug/bin'

        # Make sure we're good to go with S3 and our bucket.
        if not Helper.checkS3Info(self, self.passedBucket):
            return False

        s3 = self.parameters["s3"]

        # Get file to be compared.
        compare_path = ""
        if self.passedCompareFile is None:
            compare_path = bin_dir + "/StorMgr"  # We want our default to be big enough to cross object boundaries.
        else:
            # If our HLQ is "RESOURCES", then we'll pull the file from the SysTest framework's resources repository.
            if os.path.dirname(self.passedCompareFile) == "RESOURCES":
                compare_path = get_resource(self, os.path.basename(self.passedCompareFile))
            else:
                compare_path = self.passedCompareFile

        compare_size = os.stat(compare_path).st_size

        lkey = "large"
        if self.passedKey is not None:
            lkey = self.passedKey

        s3.keys.append(lkey)

        # Read it to a file and then compare.
        dest_path = bin_dir + "/TestS3VerifyLBLOB-loadedfile.dump"

        k = Key(s3.bucket1)
        k.key = lkey

        self.log.info("Comparing %s of size %d bytes with key %s with "
                      "Boto's Key.get_contents_to_filename() interface." %
                      (compare_path, compare_size, lkey))

        k.get_contents_to_filename(filename=dest_path)

        test_passed = filecmp.cmp(compare_path, dest_path, shallow=False)
        if not test_passed:
            md5sum = str(hashlib.md5(open(compare_path, 'rb').read()).hexdigest())
            self.log.error("File mis-match. [" + md5sum + "]")

        os.remove(dest_path)

        return test_passed


# This class contains the attributes and methods to test
# the FDS S3 interface to put an objected with verifiable content
# in place for later retrieval and validation
#
# You must have successfully created an S3 connection
# and stored it in self.parameters["s3"].conn (see TestS3IntFace.TestS3GetConn)
# and created a bucket and stored it in self.parameters["s3"].bucket1.
# If retry is True then don't log anerror message
class TestS3LoadVerifiableObject(TestCase.FDSTestCase):
    def __init__(self, parameters=None, bucket=None, seedValue="a", blobKey="s3VerifiableObject", logInfo=True, retry= False):
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_S3LoadVerifiableObject,
                                             "Load a verifiable object into S3 bucket")

        self.passedBucket = bucket
        self.passedSeedValue = seedValue
        self.passBlobKey = blobKey
        self.passedLogInfo = logInfo
        self.passedRetry = retry

    def test_S3LoadVerifiableObject(self):
        """
        Test Case:
        Attempt to load an object with verifiable contents into a bucket
        """

        # Make sure we're good to go with S3 and our bucket.
        if not Helper.checkS3Info(self, self.passedBucket):
            return False

        if self.passedLogInfo:
            self.log.info("Load a blob with verifiable contents using key {0}_{1} into an S3 bucket.".
                          format(self.passBlobKey, self.passedSeedValue))
        bucket = self.parameters["s3"].bucket1

        verifiable_file_contents = self.passedSeedValue * 1024
        try:
            verifiable_object = bucket.new_key('{0}_{1}'.format(self.passBlobKey, self.passedSeedValue))
            verifiable_object.set_contents_from_string(verifiable_file_contents)
        except Exception as e:
            if self.passedRetry is False:
                self.log.error("Failed to create S3 blob with key {0}_{1}".format(self.passBlobKey, self.passedSeedValue))
                self.log.error(e.message)
            else:
                self.log.info("Retry S3LoadVerifiableObject")
            return False
        else:
            # Capture the hash for verification
            stored_hash = hashlib.sha1(verifiable_file_contents).hexdigest()
            self.parameters["s3"].verifiers['{0}_{1}'.format(self.passBlobKey, self.passedSeedValue)] = stored_hash

            return True


# This class contains the attributes and methods to test
# the FDS S3 interface to verify an object with verifiable content
#
# You must have successfully created an S3 connection
# and stored it in self.parameters["s3"].conn (see TestS3IntFace.TestS3GetConn)
# and created a bucket and stored it in self.parameters["s3"].bucket1.
# If retry is True then dont log an error message
class TestS3CheckVerifiableObject(TestCase.FDSTestCase):
    def __init__(self, parameters=None, bucket=None, seedValue="a", blobKey="s3VerifiableObject", logInfo=True, retry=False):
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_S3CheckVerifiableObject,
                                             "Check verifiable object in S3 bucket")

        self.passedBucket = bucket
        self.passedSeedValue = seedValue
        self.passedBlobKey = blobKey
        self.passedLogInfo = logInfo
        self.passedRetry = retry
    def test_S3CheckVerifiableObject(self):
        """
        Test Case:
        Attempt to get an object with verifiable contents and validate the
        checksum matches what we have stored
        """

        # Make sure we're good to go with S3 and our bucket.
        if not Helper.checkS3Info(self, self.passedBucket):
            return False

        if self.passedLogInfo:
            self.log.info("Verify contents of object using key {0}_{1} in an S3 bucket.".
                          format(self.passedBlobKey, self.passedSeedValue))
        bucket = self.parameters["s3"].bucket1

        test_passed = False
        try:
            verifiable_object = bucket.get_key(key_name='{0}_{1}'.format(self.passedBlobKey, self.passedSeedValue))
            verify_hash = hashlib.sha1(verifiable_object.get_contents_as_string()).hexdigest()
        except Exception as e:
            self.log.warning("Could not get object to be verified with key <{0}_{1}>".
                           format(self.passedBlobKey, self.passedSeedValue))
            self.log.warning(e.message)
        else:
            stored_verify_hash = self.parameters['s3'].verifiers['{0}_{1}'.format(self.passedBlobKey, self.passedSeedValue)]
            if stored_verify_hash == verify_hash:
                test_passed = True
            else:
                self.log.info("Hash of object read with key <{0}_{1}>: {2}".
                              format(self.passedBlobKey, self.passedSeedValue, verify_hash))
                self.log.info("Hash of object stored from LoadVerifiableObject: %s " % stored_verify_hash)
                if self.passedRetry is False:
                    self.log.error("S3 Verifiable Object hash did not match")
                else:
                    self.log.info("Retry S3CheckVerifiableObject")

        return test_passed


# This class contains the attributes and methods to test
# the FDS S3 interface to delete an object with verifiable content
#
# You must have successfully created an S3 connection
# and stored it in self.parameters["s3"].conn (see TestS3IntFace.TestS3GetConn)
# and created a bucket and stored it in self.parameters["s3"].bucket1.
# If retry is True then dont log an error message.
class TestS3DeleteVerifiableObject(TestCase.FDSTestCase):
    def __init__(self, parameters=None, bucket=None, seedValue="a", verify=True, logInfo=True, retry=False):
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_S3DeleteVerifiableObject,
                                             "Delete verifiable object in S3 bucket")

        self.passedBucket=bucket
        self.passedSeedValue=seedValue
        self.passedVerify = verify
        self.passedLogInfo = logInfo
        self.passedRetry = retry

    def test_S3DeleteVerifiableObject(self):
        """
        Test Case:
        Attempt to delete an object.
        """

        # Make sure we're good to go with S3 and our bucket.
        if not Helper.checkS3Info(self, self.passedBucket):
            return False

        if self.passedLogInfo:
            self.log.info("Delete object seeded with <{0}> from an S3 bucket.".format(self.passedSeedValue))
        bucket = self.parameters["s3"].bucket1

        test_passed = False
        try:
            deleted_object_key = bucket.delete_key('s3VerifiableObject_{0}'.format(self.passedSeedValue))
        except Exception as e:
            if self.passedRetry is False:
                self.log.error("Could not delete object with key <s3VerifiableObject_{0}>".
                           format(self.passedSeedValue))
                self.log.error(e.message)
            else:
                self.log.info("Retry S3DeleteVerifiableObject")
        else:
            if self.passedVerify:
                # Verify the delete.
                checkObject = TestS3CheckVerifiableObject(self.parameters, bucket=self.passedBucket,
                                                          seedValue=self.passedSeedValue, retry=self.passedRetry)
                if not checkObject.test_S3CheckVerifiableObject():
                    if self.passedLogInfo:
                        self.log.info("Verified delete of object with key <s3VerifiableObject_{0}>".
                                         format(self.passedSeedValue))
                    test_passed = True
            else:
                test_passed = True

        return test_passed


# This class contains the attributes and methods to test
# the FDS S3 interface in a loop of creating, reading and deleting an object with verifiable content
#
# You must have successfully created an S3 connection
# and stored it in self.parameters["s3"].conn (see TestS3IntFace.TestS3GetConn)
# and created a bucket and stored it in self.parameters["s3"].bucket1.
#
# runTime: The amount of time in which to run the loop.
# retry: Whether, upon an I/O failure, the I/O should be re-attempted
# retryMax: Maximum number of retries allowed.
# verifyDelete: Whether a DELETE should be verified by following it with a query and expecting failure.
class TestS3VerifiableObjectLoop(TestCase.FDSTestCase):
    def __init__(self, parameters=None, bucket=None, runTime=60.0, retry="false", retryMax=10, verifyDelete="true"):
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_S3VerifiableObjectLoop,
                                             "Loop verifiable object in S3 bucket")

        self.passedBucket = bucket
        self.passedRunTime = runTime
        if retry == "true":
            self.passedRetry = True
        else:
            self.passedRetry = False

        self.passedRetryMax = retryMax

        if verifyDelete == "true":
            self.passedVerifyDelete = True
        else:
            self.passedVerifyDelete = False

        self.loopControl = "start"

    def timeout(self):
        self.log.info("Stop loop on objects for {0} seconds, timeout.".format(self.passedRunTime))
        self.loopControl = "stop"

    def test_S3VerifiableObjectLoop(self):
        """
        Test Case:
        Attempt to loop on creating, reading and deleting an object.
        """

        # Make sure we're good to go with S3 and our bucket.
        if not Helper.checkS3Info(self, self.passedBucket):
            return False

        logging.getLogger('boto').setLevel(logging.CRITICAL)

        self.log.info("Loop on objects for {0} seconds.".format(float(self.passedRunTime)))
        t = Timer(float(self.passedRunTime), self.timeout)
        t.start()

        test_passed = True
        seedOrd = 0
        while (self.loopControl != "stop") and (test_passed):
            seedChr = chr(ord('A') + (seedOrd % (ord('z') - ord('A') + 1)))

            if (self.loopControl != "stop") and (test_passed):
                objectCreate = TestS3LoadVerifiableObject(parameters=self.parameters, bucket=self.passedBucket,
                                                          seedValue=seedChr, logInfo=(seedOrd % 100 == 0), retry=self.passedRetry)
                retryCnt = self.passedRetryMax
                while (retryCnt > 0):
                    test_passed = objectCreate.test_S3LoadVerifiableObject()
                    if test_passed:
                        break
                    elif self.passedRetry:
                        self.log.warning("Retry.")
                        retryCnt -= 1
                    else:
                        break

            if (self.loopControl != "stop") and (test_passed):
                objectRead = TestS3CheckVerifiableObject(parameters=self.parameters, bucket=self.passedBucket,
                                                         seedValue=seedChr, logInfo=(seedOrd % 100 == 0), retry=self.passedRetry)
                retryCnt = self.passedRetryMax
                while (retryCnt > 0):
                    test_passed = objectRead.test_S3CheckVerifiableObject()
                    if test_passed:
                        break
                    elif self.passedRetry:
                        self.log.warning("Retry.")
                        retryCnt -= 1
                    else:
                        break

            if (self.loopControl != "stop") and (test_passed):
                objectDelete = TestS3DeleteVerifiableObject(parameters=self.parameters, bucket=self.passedBucket,
                                                            seedValue=seedChr, verify=self.passedVerifyDelete,
                                                            logInfo=(seedOrd % 100 == 0), retry=self.passedRetry)
                retryCnt = self.passedRetryMax
                while (retryCnt > 0):
                    test_passed = objectDelete.test_S3DeleteVerifiableObject()
                    if test_passed:
                        break
                    elif self.passedRetry:
                        self.log.warning("Retry.")
                        retryCnt -= 1
                    else:
                        break

            seedOrd += 1

        t.cancel()

        return test_passed


# This class contains the attributes and methods to test
# the FDS S3 interface in a loop of creating many blobs with verifiable content
#
# You must have successfully created an S3 connection
# and stored it in self.parameters["s3"].conn (see TestS3IntFace.TestS3GetConn)
# and created a bucket and stored it in self.parameters["s3"].bucket1.
#
# numBlobs: The number of Blobs to be created
class TestS3VerifiableBlobCreate(TestCase.FDSTestCase):
    def __init__(self, parameters=None, bucket=None, numBlobs=1024):
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_S3VerifiableBlobCreate,
                                             "Create verifiable blobs in S3 bucket")

        self.passedBucket = bucket
        self.passedNumBlobs = int(numBlobs)

    def test_S3VerifiableBlobCreate(self):
        """
        Test Case:
        Attempt to create the specified number of blobs.
        """

        # Make sure we're good to go with S3 and our bucket.
        if not Helper.checkS3Info(self, self.passedBucket):
            return False

        logging.getLogger('boto').setLevel(logging.CRITICAL)

        self.log.info("CREATE {} blobs.".format(self.passedNumBlobs))

        test_passed = True
        blobCnt = 0
        while (blobCnt < self.passedNumBlobs) and (test_passed):
            seedChr = chr(ord('A') + (blobCnt % (ord('z') - ord('A') + 1)))

            objectCreate = TestS3LoadVerifiableObject(parameters=self.parameters, bucket=self.passedBucket,
                                                      blobKey="blob{}".format(blobCnt), seedValue=seedChr,
                                                      logInfo=(blobCnt % 100 == 0))
            test_passed = objectCreate.test_S3LoadVerifiableObject()

            if not test_passed:
                break

            blobCnt += 1

        return test_passed


# This class contains the attributes and methods to test
# the FDS S3 interface in a loop of randomly READing blobs
# generated with TestS3VerifiableBlobCreate.
#
# You must have successfully created an S3 connection
# and stored it in self.parameters["s3"].conn (see TestS3IntFace.TestS3GetConn)
# and created a bucket and stored it in self.parameters["s3"].bucket1.
#
# runTime: The amount of time in which to run the READs.
# retry: Whether, upon a READ failure, the READ should be re-attempted
# retryMax: Maximum number of retries allowed.
# numBlobs: The number of Blobs to work with.
# allowMisses: Whether or not to count failed READs are a test case failure. True if not.
class TestS3VerifiableBlobRead(TestCase.FDSTestCase):
    def __init__(self, parameters=None, bucket=None, runTime=60.0, retry="false", retryMax=2, numBlobs=1024, allowMisses="false"):
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_S3VerifiableBlobRead,
                                             "Randomly reading verifiable blobs in S3 bucket")

        self.passedBucket = bucket
        self.passedRunTime = runTime
        if retry == "true":
            self.passedRetry = True
        else:
            self.passedRetry = False

        self.passedRetryMax = retryMax
        self.passedNumBlobs = int(numBlobs)

        if allowMisses == "true":
            self.passedAllowMisses = True
        else:
            self.passedAllowMisses = False

        self.loopControl = "start"

    def timeout(self):
        self.log.info("Stop loop on blob READ for {0} seconds, timeout.".format(self.passedRunTime))
        self.loopControl = "stop"

    def test_S3VerifiableBlobRead(self):
        """
        Test Case:
        Attempt to randomly READ blobs for the specified amount of time.
        """

        # Make sure we're good to go with S3 and our bucket.
        if not Helper.checkS3Info(self, self.passedBucket):
            return False

        logging.getLogger('boto').setLevel(logging.CRITICAL)

        self.log.info("Loop on blob READs for {0} seconds.".format(float(self.passedRunTime)))
        t = Timer(float(self.passedRunTime), self.timeout)
        t.start()

        test_passed = True
        loopCnt = 0
        while (self.loopControl != "stop") and (test_passed):
            blobID = random.randrange(0, self.passedNumBlobs)
            seedChr = chr(ord('A') + (blobID % (ord('z') - ord('A') + 1)))

            if (self.loopControl != "stop") and (test_passed):
                objectRead = TestS3CheckVerifiableObject(parameters=self.parameters, bucket=self.passedBucket,
                                                         blobKey="blob{}".format(blobID), seedValue=seedChr,
                                                         logInfo=(loopCnt % 100 == 0), retry=self.passedRetry)
                # Capture the hash of the blob we expect for verification
                verifiable_file_contents = seedChr * 1024
                stored_hash = hashlib.sha1(verifiable_file_contents).hexdigest()
                self.parameters["s3"].verifiers['{0}_{1}'.format("blob{}".format(blobID), seedChr)] = stored_hash

                retryCnt = self.passedRetryMax
                while (retryCnt > 0):
                    test_passed = objectRead.test_S3CheckVerifiableObject()

                    if self.passedAllowMisses:
                        test_passed = True

                    if test_passed:
                        break
                    elif self.passedRetry:
                        self.log.warning("Retry.")
                        retryCnt -= 1
                    else:
                        break

            loopCnt += 1

        t.cancel()

        return test_passed


# This class contains the attributes and methods to test
# the FDS S3 interface to list the keys of a bucket.
#
# You must have successfully created an S3 connection
# and stored it in self.parameters["s3"].conn (see TestS3IntFace.TestS3GetConn)
# and created a bucket and stored it in self.parameters["s3"].bucket1.
class TestS3ListBucketKeys(TestCase.FDSTestCase):
    def __init__(self, parameters=None):
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_S3ListBucketKeys,
                                             "Listing the keys of an S3 bucket")

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
# the FDS S3 interface to delete a specific key from a bucket.
#
# You must have successfully created an S3 connection
# and stored it in self.parameters["s3"].conn (see TestS3IntFace.TestS3GetConn)
# and created a bucket and stored it in self.parameters["s3"].bucket1 or passed it in.
#
# @param bucket The bucket from which to delete the key.
# @param key The key to be deleted.
# @param verify "true" if the delete should have deleted something and you want to
#               know if it did not. "false" if you don't care.
class TestS3DelKey(TestCase.FDSTestCase):
    def __init__(self, parameters=None, bucket=None, key=None, verify="false"):
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_S3DelKey,
                                             "Deleting a specific key of an S3 bucket")

        self.passedBucket = bucket
        self.passedKey = key
        self.passedVerify = verify

    def test_S3DelKey(self):
        """
        Test Case:
        Attempt to delete the given key from an S3 Bucket.
        """

        if not Helper.checkS3Info(self, self.passedBucket):
            return False

        s3 = self.parameters["s3"]
        self.log.info("Delete key [{}] from bucket [{}]".format(self.passedKey, self.passedBucket))

        try:
            k = Key(s3.bucket1, self.passedKey)
            k.delete()
        except:
            if self.passedVerify:
                self.log.error('Delete failed for key [{}]'.format(self.passedKey))
                return False

        return True



# This class contains the attributes and methods to test
# the FDS S3 interface to delete all the keys of a bucket.
#
# You must have successfully created an S3 connection
# and stored it in self.parameters["s3"].conn (see TestS3IntFace.TestS3GetConn)
# and created a bucket and stored it in self.parameters["s3"].bucket1.
class TestS3DelBucketKeys(TestCase.FDSTestCase):
    def __init__(self, parameters=None, bucket=None):
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_S3DelBucketKeys,
                                             "Deleting all the keys of an S3 bucket")

        self.passedBucket = bucket
    def test_S3DelBucketKeys(self):
        """
        Test Case:
        Attempt to delete all the keys of an S3 Bucket.
        """

        if not Helper.checkS3Info(self, self.passedBucket):
            return False

        s3 = self.parameters["s3"]
        self.log.info("Delete all the keys from [{}]".format(s3.bucket1))

        for key in s3.bucket1.list():
            key.delete()

        for key in s3.bucket1.list():
            self.log.error("Unexpected keys remaining in bucket: %s" % key.name)
            return False

        return True


# This class contains the attributes and methods to test
# the FDS S3 interface to delete a bucket.
#
# You must have successfully created an S3 connection
# and stored it in self.parameters["s3"].conn (see TestS3IntFace.TestS3GetConn)
# and created a bucket and stored it in self.parameters["s3"].bucket1.
class TestS3DelBucket(TestCase.FDSTestCase):
    def __init__(self, parameters=None, bucket=None):
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_S3DelBucket,
                                             "Deleting an S3 bucket")
        self.passedBucket = bucket
    def test_S3DelBucket(self):
        """
        Test Case:
        Attempt to delete an S3 Bucket.
        """

        if not Helper.checkS3Info(self, self.passedBucket):
            return False
        else:
            s3 = self.parameters["s3"]
            self.log.info("Delete S3 bucket : {}".format(s3.bucket1))

            s3.conn.delete_bucket(self.parameters["s3"].bucket1.name)

            if s3.conn.lookup(self.passedBucket) != None:
                self.log.error("Unexpected bucket: {}".format(self.passedBucket))
                return False

            s3.bucket1 = None
            return True


class TestPuts(TestCase.FDSTestCase):
    def __init__(self, parameters=None, bucket=None, dataset=1, count=10, size='1K', fail=False):
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_Puts,
                                             "upload N no.of objects with specified size")
        self.dataset = str(dataset)
        self.count   = int(count)
        self.size    = size
        self.passedBucket=bucket
        self.fail    = fail

    def test_Puts(self):
        if not Helper.checkS3Info(self, self.passedBucket):
            return False

        s3 = self.parameters["s3"]
        if self.dataset not in s3.verifiers:
            s3.verifiers[self.dataset] = {}
            s3.verifiers[self.dataset]['count'] = self.count
            s3.verifiers[self.dataset]['size'] = self.size
        else:
            self.count = s3.verifiers[self.dataset]['count']
            self.size = s3.verifiers[self.dataset]['size']

        self.log.info("uploading {} keys of size: {} onto [{}]".format(self.count, self.size, s3.bucket1))
        for n in range(0, self.count):
            key = Helper.keyName(self.dataset + str(n))
            #self.log.info('uploading key {} : {}'.format(n, key))
            value = Helper.genData(self.size,n)
            self.parameters["s3"].verifiers[self.dataset][key] = hash(value)
            try:
                k = s3.bucket1.new_key(key)
                k.set_contents_from_string(value)
                if self.fail:
                    self.log.error('Put should have failed but succeeded: {}'.format(key))
                    return False
            except:
                if not self.fail:
                    self.log.error('Put failed on : {}'.format(key))
                    raise

        return True

class TestGets(TestCase.FDSTestCase):
    def __init__(self, parameters=None, bucket=None, dataset=1, count=10, size='1K', fail=False):
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_Gets,
                                             "fetch N no.of objects with specified size")
        self.dataset = str(dataset)
        self.count   = int(count)
        self.size    = size
        self.passedBucket=bucket
        self.fail    = fail

    def test_Gets(self):
        if not Helper.checkS3Info(self, self.passedBucket):
            return False

        s3 = self.parameters["s3"]
        if self.dataset not in s3.verifiers:
            s3.verifiers[self.dataset] = {}
            s3.verifiers[self.dataset]['count'] = self.count
            s3.verifiers[self.dataset]['size'] = self.size
        else:
            self.count = s3.verifiers[self.dataset]['count']
            self.size = s3.verifiers[self.dataset]['size']

        self.log.info("fetching {} keys of size: {} from [{}]".format(self.count, self.size, s3.bucket1))

        for n in range(0, self.count):
            key = Helper.keyName(self.dataset + str(n))
            #self.log.info('fetching key {} : {}'.format(n, key))
            try:
                value = s3.bucket1.get_key(key).get_contents_as_string()
                valuehash = hash(value)
                if self.dataset in self.parameters["s3"].verifiers and key not in self.parameters["s3"].verifiers[self.dataset]:
                    # key hash is missing .. will get the hash
                    expectedvalue = Helper.genData(self.size,n)
                    self.parameters["s3"].verifiers[self.dataset][key] = hash(expectedvalue)

                if self.dataset in self.parameters["s3"].verifiers and key in self.parameters["s3"].verifiers[self.dataset]:
                    if self.parameters["s3"].verifiers[self.dataset][key] != valuehash:
                        self.log.error('hash mismatch for key {} : {} '.format(n, key))
                        return False
                if self.fail:
                    self.log.error('Get should have failed but succeeded : {}'.format(key))
                    return False
            except:
                if not self.fail:
                    self.log.error('Get failed on : {}'.format(key))
                    raise
        return True

class TestDeletes(TestCase.FDSTestCase):
    def __init__(self, parameters=None, bucket=None, dataset=1, count=10, fail=False):
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_Deletes,
                                             "check N no.of keys")
        self.dataset = str(dataset)
        self.count   = int(count)
        self.passedBucket=bucket
        self.fail    = fail

    def test_Deletes(self):
        if not Helper.checkS3Info(self, self.passedBucket):
            return False

        s3 = self.parameters["s3"]
        if self.dataset not in s3.verifiers:
            s3.verifiers[self.dataset] = {}
            s3.verifiers[self.dataset]['count'] = self.count
        else:
            self.count = s3.verifiers[self.dataset]['count']

        self.log.info("deleting {} keys from [{}]".format(self.count, s3.bucket1))

        for n in range(0, self.count):
            key = Helper.keyName(self.dataset + str(n))
            #self.log.info('deleting key {} : {}'.format(n, key))
            try:
                k = Key(s3.bucket1, key)
                k.delete()
                if self.fail:
                    self.log.error('Delete should have failed but succeeded : {}'.format(key))
                    return False
            except:
                if not self.fail:
                    self.log.error('Delete failed on : {}'.format(key))
                    raise

        return True

class TestKeys(TestCase.FDSTestCase):
    def __init__(self, parameters=None, bucket=None, dataset=1, count=10, exist=True):
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_Keys,
                                             "delete N no.of objects")
        self.dataset = str(dataset)
        self.count   = int(count)
        self.exist   = Helper.boolean(exist)
        self.passedBucket=bucket

    def test_Keys(self):
        if not Helper.checkS3Info(self, self.passedBucket):
            return False

        s3 = self.parameters["s3"]
        if self.dataset not in s3.verifiers:
            s3.verifiers[self.dataset] = {}
            s3.verifiers[self.dataset]['count'] = self.count
        else:
            self.count = s3.verifiers[self.dataset]['count']

        self.log.info("checking {} keys in [{}]".format(self.count, s3.bucket1))
        bucket_keys = [key.name for key in s3.bucket1.list()]
        for n in range(0, self.count):
            key = Helper.keyName(self.dataset + str(n))
            key_exists = key in bucket_keys
            if self.exist != key_exists:
                self.log.error("key {} exist check failed. Should exist: {}, Does exist: {}".format(key, self.exist, key_exists))
                return False

        return True



# This class contains the attributes and methods to reset
# the S3 object so that we can reuse these test cases.
#
# You must have successfully created a connection.
class TestS3ObjReset(TestCase.FDSTestCase):
    def __init__(self, parameters=None):
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_S3ObjReset,
                                             "Resetting S3 object")

    def test_S3ObjReset(self):
        """
        Test Case:
        Reset teh S3 object.
        """

        if not "s3" in self.parameters:
            self.log.error("No S3 interface object to reset.")
        else:
            self.log.info("Reset S3 object.")
            del self.parameters["s3"]

        return True

if __name__ == '__main__':
    TestCase.FDSTestCase.fdsGetCmdLineConfigs(sys.argv)
    unittest.main()
