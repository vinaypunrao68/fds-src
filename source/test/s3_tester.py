#
# Copyright 2013 Formation Data Systems, Inc.
#
# Tests basic Amazon S3 functionality.
import boto
from boto.s3.connection import S3Connection, OrdinaryCallingFormat
import unittest
import optparse
import pdb
import traceback

verbose  = False
debug    = False
numObjs  = None
numBucks = None
numConns = None
hostName = None
hostPort = None

#
# Describes s3 testing paramters
#
class S3Tester():
    s3Connections  = []
    numConnections = 1
    host           = "s3.amazonaws.com"
    port           = None
    bucketsPerConn = 2
    objectsPerConn = 5
    namePrefix     = "fds-s3-tester-"
    bucketPrefix   = "bucket-"
    objectPrefix   = "object-"

    def __init__(self,
                 _host  = None,
                 _port  = None,
                 _conns = None,
                 _bucks = None,
                 _objs  = None):
        if _host != None:
            self.host = _host
        if _port != None:
            self.port = int(_port)
        if _conns != None:
            self.numConnections = int(_conns)
        if _bucks != None:
            self.bucketsPerConn = int(_bucks)
        if _objs != None:
            self.objectsPerConn = int(_objs)

    #
    # Opens remote host connections
    #
    def openConns(self):
        for i in range(0, self.numConnections):
            if self.port == None:
                self.s3Connections.append(boto.connect_s3(host=self.host,
                                                          proxy=self.host,
                                                          calling_format = OrdinaryCallingFormat(),
                                                          is_secure=False))
            else:
                self.s3Connections.append(boto.connect_s3(host=self.host,
                                                          proxy=self.host,
                                                          proxy_port=self.port,
                                                          calling_format = OrdinaryCallingFormat(),
                                                          is_secure=False))
            assert(self.s3Connections[i] != None)

    #
    # Closes remote host connections
    #
    def closeConns(self):
        for s3Conn in self.s3Connections:
            s3Conn.close()
            self.s3Connections.remove(s3Conn)

    def getNumConns(self):
        return self.numConnections

    def getNumBuckets(self):
        return self.bucketsPerConn

    def getNumObjects(self):
        return self.objectsPerConn

    #
    # Returns a specific connection instance
    #
    def getConn(self, index):
        if index > len(self.s3Connections):
            return None
        return self.s3Connections[index]

    #
    # Gets all of the buckets in a specific
    # connection instance
    #
    def getAllBuckets(self, index):
        s3Conn = self.getConn(index)
        if s3Conn == None:
            return None
        buckets = s3Conn.get_all_buckets()
        print "S3 connection", index , "has buckets", buckets
        return buckets

    #
    # Creates a bucket in a specific
    # connection instance
    #
    def createBucket(self, index, name):
        s3Conn = self.getConn(index)
        if s3Conn == None:
            return None
        bucketName = self.namePrefix + self.bucketPrefix + name
        bucket = s3Conn.create_bucket(bucketName)
        print "In S3 connection", index, "created bucket", bucket
        return bucket

    #
    # Deletes a bucket in a specific
    # connection instance
    #
    def deleteBucket(self, index, bucket):
        s3Conn = self.getConn(index)
        if s3Conn == None:
            return None
        try:
            result = s3Conn.delete_bucket(bucket)
            print "In S3 connection", index, "deleted bucket", bucket
        except Exception, e:
            print "*** Caught exception %s: %s" % (e.__class__, e)
            traceback.print_exc()
            return None
        return True

    #
    # Puts an object in a bucket in a specific
    # connection instance
    #
    def putObject(self, index, bucket, name, data):
        keyFactory = boto.s3.key.Key(bucket)
        keyFactory.key = name
        bytesPut = keyFactory.set_contents_from_string(data)
        print "In S3 connection", index, "put", bytesPut, "bytes for data", data
        keyFactory.close()
        return True

    #
    # Gets an object from a bucket in a specific
    # connection instance
    #
    def getObject(self, index, bucket, name):
        keyFactory = boto.s3.key.Key(bucket)
        keyFactory.key = name
        data = keyFactory.get_contents_as_string()
        print "In S3 connection", index, "got", data
        keyFactory.close()
        return data

    #
    # Delete an object from a bucket in a specific
    # connection instance
    #
    def deleteObject(self, index, bucket, name):
        keyFactory = boto.s3.key.Key(bucket)
        keyFactory.key = name
        keyFactory.delete()
        print "In S3 connection", index, "deleted key", name
        keyFactory.close()
        return True

#
# Describes tests and test helpers
#
class TestFunctions(unittest.TestCase):
    s3test = None

    def __init__(self,
                 methodName='runTest'):
        unittest.TestCase.__init__(self, methodName)
        self.s3Test = S3Tester(_host=hostName,
                               _port=hostPort,
                               _conns=numConns,
                               _bucks=numBucks,
                               _objs=numObjs)

    def setUp(self):
        self.s3Test.openConns()
        print "Setup complete"

    def tearDown(self):
        self.s3Test.closeConns()
        print "Teardown complete"

    def beginTestHdr(self, testName):
        print "********** Beginning test: %s **********" % (testName)

    def endTestHdr(self, testName):
        print "********** Ending test: %s **********" % (testName)
    
    #
    # Tests listing buckets in a remote host account
    #
    def test_a_listBucket(self):
        testName = "Bucket list"
        self.beginTestHdr(testName)

        for i in range(0, self.s3Test.getNumConns()):
            buckets = self.s3Test.getAllBuckets(i)
            self.assertTrue(buckets != None)

        self.endTestHdr(testName)

    #
    # Tests creating/deleting buckets
    #
    def test_b_crtDelBucket(self):
        testName = "Bucket create/delete"
        self.beginTestHdr(testName)

        for connId in range(0, self.s3Test.getNumConns()):
            for buckId in range(0, self.s3Test.getNumBuckets()):
                bucket = self.s3Test.createBucket(connId, "test-bucket%d" % (buckId))
                self.assertTrue(bucket != None)
                result = self.s3Test.deleteBucket(connId, bucket)
                self.assertTrue(result != None)

        self.endTestHdr(testName)

    #
    # Tests creating/deleting buckets and
    # creating/getting/deleting objects in those
    # buckets
    #
    def test_c_putObjs(self):
        testName = "Object put"
        self.beginTestHdr(testName)

        keys = []
        data = []
        for connId in range(0, self.s3Test.getNumConns()):
            bucket = self.s3Test.createBucket(connId, "test-put-obj%d" % (connId))
            self.assertTrue(bucket != None)

            for objId in range(0, self.s3Test.getNumObjects()):
                keys.append("object%d" % (objId))
                data.append("Hi, I'm object data%d" % (objId))
                result = self.s3Test.putObject(connId,
                                               bucket,
                                               keys[objId],
                                               data[objId])
                self.assertTrue(result != None)

            for objId in range(0, len(keys)):
                getData = self.s3Test.getObject(connId,
                                                bucket,
                                                keys[objId])
                self.assertTrue(getData == data[objId])

            for objId in range(0, len(keys)):
                result = self.s3Test.deleteObject(connId,
                                                  bucket,
                                                  keys[objId])
                self.assertTrue(result != None)
                
            result = self.s3Test.deleteBucket(connId, bucket)
            self.assertTrue(result != None)

        self.endTestHdr(testName)
            

if __name__ == '__main__':
    #
    # Parse command line options
    #
    parser = optparse.OptionParser("usage: %prog [options]")
    parser.add_option("-v", "--verbose", action="store_true",
                      dest="verbose", help="enable verbosity")
    parser.add_option("-d", "--debug", action="store_true",
                      dest="debug", help="enable debug breaks")
    parser.add_option("-t", "--test", dest="testName",
                      help="test to run", metavar="STRING")
    parser.add_option("-o", "--objects", dest="objects",
                      help="number of objects", metavar="INT")
    parser.add_option("-b", "--buckets", dest="buckets",
                      help="number of buckets", metavar="INT")
    parser.add_option("-c", "--connections", dest="connections",
                      help="number of connections", metavar="INT")
    parser.add_option("-p", "--port", dest="port",
                      help="s3 remote host port", metavar="INT")
    parser.add_option("-r", "--host", dest="host",
                      help="s3 remote hostname", metavar="STRING")

    (options, args) = parser.parse_args()
    testName = options.testName
    verbose  = options.verbose
    debug    = options.debug
    numObjs  = options.objects
    numBucks = options.buckets
    numConns = options.connections
    hostName = options.host
    hostPort = options.port

    verbosity = 0
    if verbose == True:
        verbosity = 2

    #
    # Run test(s)
    #
    suite = unittest.TestLoader().loadTestsFromTestCase(TestFunctions)
    if testName != None:
        suite = unittest.TestSuite()
        suite.addTest(TestFunctions(testName))

    unittest.TextTestRunner(verbosity=verbosity).run(suite)
        
