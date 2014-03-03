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
import threading
import time
import random

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

    ## S3 workload generator
    s3wkld     = None
    ## Service config controller
    srvCfg     = None
    ## Running workloads threads
    workloads  = []
    ## Data written during test
    putData    = {}

    ## Name of node sections already deployed
    deployedNodeSections = []

    def __init__(self, cfgFile, verbose, debug):
        self.numPuts    = 10
        self.numGets    = 1000
        self.maxObjSize = 4 * 1024 * 1024

        self.numConns   = 1
        self.numBuckets = 1
        self.numObjs    = 100

        self.cfgFile    = cfgFile
        self.verbose    = verbose
        self.debug      = debug

        self.dataGen    = ServiceWkld.GenObjectData(self.maxObjSize)
        # TODO: Currently hard coded host/port
        # TODO: Move to when we know which AM we want to connect to
        self.s3wkld     = ServiceWkld.S3Wkld("localhost", 8000,
                                             False, False,
                                             self.numConns,
                                             self.numBuckets,
                                             self.numObjs)

        #
        # Load the configuration files
        #
        self.srvCfg     = ServiceConfig.TestBringUp(self.cfgFile, verbose, debug)
        self.srvCfg.loadCfg()

    ## Deploys initial cluster nodes and config
    #
    # This is meant
    def initialDeploy(self):
        # Determine which node name host an OM
        # and bring that node up
        omSection = self.srvCfg.getOmNodeSection()
        result = self.srvCfg.bringUpSection(omSection)
        if result != 0:
            print "Failed to bring up %s" % (omSection)
            return result
        self.deployedNodeSections.append(omSection)

        # Bring up all clients
        result = self.srvCfg.bringUpClients()
        if result != 0:
            print "Failed to bring up clients"
            return result

        # Bring up all volume policies
        result = self.srvCfg.bringUpPols()
        if result != 0:
            print "Failed to bring up volume policies"
            return result

        # Bring up all volumes
        result = self.srvCfg.bringUpVols()
        if result != 0:
            print "Failed to bring up volumes"
            return result

        return 0

    ## Returns true if a node section is already deployed
    #
    def isNodeDeployed(self, nodeSection):
        for deployedSection in self.deployedNodeSections:
            if deployedSection == nodeSection:
                return True
        return False

    ## Deploy an undeployed node
    #
    # Choose just the next undeployed node
    # and deploys it
    def nextNodeDeploy(self, service=None):
        # Find the next undeployed node section
        nodeSections = self.srvCfg.getNodeSections()
        for section in nodeSections:
            if self.isNodeDeployed(section) == False:
                result = self.srvCfg.bringUpSection(section, service)
                if result != 0:
                    print "Failed to deploy %s" % (section)
                    return result
                return 0
        # No more nodes to deploy
        return -1

    ## Undeploy a deployed node
    #
    # Chose next node to undeploy
    def nextNodeUndeploy(self, service=None):
        # Find non-OM node to undeploy
        nodeSections = self.srvCfg.getNodeSections()
        for section in nodeSections:
            if self.isNodeDeployed(section) == True:
                if self.isNodeSectionOm(section) == False:
                    # result = self.srvCfg.bringUpSection(section, service)
                    result = 0
                    if result != 0:
                        print "Failed to undeploy %s" % (section)
                        return result
                    return 0
        # No non-OM nodes to undeploy
        return -1

    ## Complete undeploy
    #
    # Undeploys everything in the cluster
    def fullUndeploy(self):
        # Bring up all clients
        result = self.srvCfg.bringDownClients()
        if result != 0:
            print "Failed to bring down clients"

        # Bring down all nodes
        result = self.srvCfg.bringDownNodes()
        if result != 0:
            print "Failed to bring down nodes"

    ## S3 workload to run in background
    #
    def s3Workload(self):
        print "Started s3 workload"
        # Local cache of what was put, used to verify gets
        self.putData = {}

        self.s3wkld.openConns()

        bucket = self.s3wkld.createBucket(0, "multi-node-bucket")
        for i in range(0, self.numPuts):
            objName = "object%d" % (i)
            data    = self.dataGen.genRandData()
            result  = self.s3wkld.putObject(0, bucket, objName, data)
            if result != True:
                print "Failed to put object %s" % (objName)
                assert(0)
            else:
                print "Put object %s of size %d" % (objName, len(data))
                self.putData[objName] = data

        keys = self.putData.keys()
        for i in range(0, self.numGets):
            # Select a random object to read
            objName = keys[random.randrange(0, len(keys))]
            data    = self.s3wkld.getObject(0, bucket, objName)
            if data != self.putData[objName]:
                print "Failed to get correct data for object %s" % (objName)
                assert(0)

            print "Got %d bytes for object %s" % (len(data), objName)
            

        self.s3wkld.closeConns()
        print "Finished s3 workload"

    ## Incremental S3 workload to run in background
    #
    # This workload assumes an initial, possibly larger,
    # workload has already populated the system.
    def incS3Workload(self):
        print "Started incremental s3 workload"

    ## Starts an background s3 workload
    #
    def startS3Workload(self):
        s3thread = threading.Thread(target=self.s3Workload,
                                    name="s3 workload thread")
        print "Forking %s workload thread" % (s3thread.name)
        s3thread.start()
        assert(s3thread.isAlive() == True)
        self.workloads.append(s3thread)

    ## Waits for workloads to finish
    def joinWorkloads(self):
        for workload in self.workloads:
            workload.join()
            if workload.isAlive() == True:
                print "Failed to join() workload %s" % (workload.name)
            else:
                print "Joined %s" % (workload.name)
        

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

    # Setup test and config
    mnt = MultiNodeTester(cfgFile, verbose, debug)

    # Deploy the initial cluster
    result = mnt.initialDeploy()
    if result != 0:
        print "Failed to deploy initial config"
        sys.exit()

    # Start a background workload
    mnt.startS3Workload()

    # Deploy another node
    time.sleep(4)
    result = mnt.nextNodeDeploy("SM")
    if result != 0:
        print "Failed to deploy next node"

    # Undeploy node
    # result = mnt.nextNodeUndeploy("SM")

    # Wait for all workloads to finish
    mnt.joinWorkloads()

    # Undeploy the entire cluster
    mnt.fullUndeploy()

    
