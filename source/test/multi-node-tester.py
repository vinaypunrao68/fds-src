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

class MultiNodeTester():
    # ServiceCfg variables
    cfgFile    = None
    verbose    = None
    debug      = None

    # Basic workload variables
    numPuts    = None
    numGets    = None
    maxObjSize = None

    # These can be depricated when wkld
    # is cleaned up
    numConns   = None
    numBuckets = None
    numObjs    = None

    # Data generator
    dataGen    = None

    # S3 workload generator
    s3wkld     = None
    # Service config controller
    srvCfg     = None

    def __init__(self, cfgFile, verbose, debug):
        self.numPuts    = 10
        self.numGets    = 10
        self.maxObjSize = 4 * 1024 * 1024

        self.numConns   = 1
        self.numBuckets = 1
        self.numObjs    = 100

        self.cfgFile    = cfgFile
        self.verbose    = verbose
        self.debug      = debug

        #
        # Load the configuration files
        #
        bu = ServiceConfig.TestBringUp(cfgFile, verbose, debug)
        bu.loadCfg()

        self.dataGen    = ServiceWkld.GenObjectData(self.maxObjSize)
        # TODO: Currently hard coded host/port
        # TODO: Move to when we know which AM we want to connect to
        self.s3wkld     = ServiceWkld.S3Wkld("localhost", 8000,
                                             self.numConns,
                                             self.numBuckets,
                                             self.numObjs)

        #
        # Load the configuration files
        #
        self.srvCfg = ServiceConfig.TestBringUp(self.cfgFile, verbose, debug)
        self.srvCfg.loadCfg()

    ## Deploys initial cluster nodes and config
    #
    # This is meant
    def initialDeploy(self):
        # Determine which node name host an OM
        # and bring that node up
        print "Init deploy"

        # Bring up one node
        node = "node1"
        result = self.srvCfg.bringUpSection(node)
        if result != 0:
            print "Failed to bring up %s" % (node)

        # Bring up one client
        client = "sh1"
        result = self.srvCfg.bringUpSection(client)
        if result != 0:
            print "Failed to bring up %s" % (client)

    ## Complete undeploy
    #
    # Undeploys everything in the cluster
    def fullUndeploy(self):
        print "Init undeploy"

        # Bring down one client
        client = "sh1"
        result = self.srvCfg.bringDownSection(client)
        if result != 0:
            print "Failed to bring down %s" % (client)

        # Bring down one node
        node = "node1"
        result = self.srvCfg.bringDownSection(node)
        if result != 0:
            print "Failed to bring down %s" % (node)

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

    mnt = MultiNodeTester(cfgFile, verbose, debug)

    mnt.initialDeploy()

    # Create workload generator
    #numConns   = 1
    #numBuckets = 1
    #numObjs    = 10
    #s3workload = ServiceWkld.S3Wkld("localhost", 8000,
    #                                numConns, numBuckets, numObjs)

    #s3workload.openConns()

    #bucket = s3workload.createBucket(0, "multi-node-bucket")
    #for i in range(0, numPuts):
    #    objName = "object%d" % (i)
    #    data = binascii.b2a_hex(os.urandom(maxObjSize))
    #    result = s3workload.putObject(0, bucket, objName, data)
    #    if result != True:
    #        print "Failed to put data"
    #    else:
    #        print "Put some test data"

    #s3workload.closeConns()

    mnt.fullUndeploy()

    
