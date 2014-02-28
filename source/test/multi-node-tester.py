#!/usr/bin/python
#
# Copyright 2014 Formation Data Systems, Inc.
#
import optparse
import pdb
import ServiceMgr
import ServiceConfig
import ServiceWkld
import os
import binascii

verbose = False
debug   = False
section = None

numPuts    = 10
numGets    = 10
#maxObjSize = 4 * 1024 * 1024
maxObjSize = 20

if __name__ == '__main__':
    #
    # Parse command line options
    #
    parser = optparse.OptionParser("usage: %prog [options]")
    parser.add_option("-f", "--file", dest="config_file",
                      help="configuration file", metavar="FILE")
    parser.add_option("-v", "--verbose", action="store_true",
                      dest="verbose", help="enable verbosity")
    parser.add_option("-d", "--debug", action="store_true",
                      dest="debug", help="enable debug breaks")
    (options, args) = parser.parse_args()
    cfgFile = options.config_file
    verbose = options.verbose
    debug = options.debug

    #
    # Load the configuration files
    #
    bu = ServiceConfig.TestBringUp(cfgFile, verbose, debug)
    bu.loadCfg()

    # Bring up one node
    node = "node1"
    result = bu.bringUpSection(node)
    if result != 0:
        print "Failed to bring up %s" % (node)

    # Bring up one client
    client = "sh1"
    result = bu.bringUpSection(client)
    if result != 0:
        print "Failed to bring up %s" % (client)

    # Create workload generator
    numConns   = 1
    numBuckets = 1
    numObjs    = 10
    s3workload = ServiceWkld.S3Tester("localhost", 8000,
                                   numConns, numBuckets, numObjs)

    s3workload.openConns()

    bucket = s3workload.createBucket(0, "multi-node-bucket")
    for i in range(0, numPuts):
        objName = "object%d" % (i)
        data = binascii.b2a_hex(os.urandom(maxObjSize))
        result = s3workload.putObject(0, bucket, objName, data)
        if result != True:
            print "Failed to put data"
        else:
            print "Put some test data"

    # Bring up second node
    secNode = "node2"
    result = bu.bringUpSection(secNode)
    if result != 0:
        print "Failed to bring up %s" % (secNode)
        
    for i in range(numPuts + 1, numPuts * 2):
        objName = "object%d" % (i)
        data = binascii.b2a_hex(os.urandom(maxObjSize))
        result = s3workload.putObject(0, bucket, objName, data)
        if result != True:
            print "Failed to put data"
        else:
            print "Put some test data"

    s3workload.closeConns()

    # Bring down one client
    result = bu.bringDownSection(client)
    if result != 0:
        print "Failed to bring down %s" % (client)

    # Bring down one node
    result = bu.bringDownSection(secNode)
    if result != 0:
        print "Failed to bring down %s" % (secNode)

    # Bring down one node
    result = bu.bringDownSection(node)
    if result != 0:
        print "Failed to bring down %s" % (node)

    
