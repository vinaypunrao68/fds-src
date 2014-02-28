#!/usr/bin/python
#
# Copyright 2014 Formation Data Systems, Inc.
#
import optparse
import pdb
import ServiceMgr
import ServiceConfig
import ServiceWkld

verbose = False
debug   = False
section = None

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
    result = s3workload.putObject(0, bucket, "object0", "Some Data")
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
    result = bu.bringDownSection(node)
    if result != 0:
        print "Failed to bring down %s" % (node)

    
