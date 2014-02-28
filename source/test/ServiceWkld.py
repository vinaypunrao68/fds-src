#
# Copyright 2014 Formation Data Systems, Inc.
#
# Tests basic Amazon S3 functionality.
import boto
from boto.s3.connection import S3Connection, OrdinaryCallingFormat
import random
import os
import binascii

class GenObjectData():
    maxDataSize = None
    def __init__(self, maxDataSize):
        self.maxDataSize = maxDataSize

    ## Generates random data of a random size
    #
    # If no size is provided, size of data is random
    # but limited to max size
    def genRandData(self, size = None):
        if size == None:
            size = random.randrange(1, self.maxDataSize)

        data = binascii.b2a_hex(os.urandom(size))
        return data
#
# Describes s3 testing paramters
#
class S3Wkld():
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
