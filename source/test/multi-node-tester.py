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
import multiprocessing
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

    ## S3 workload generator
    s3wkld     = None
    ## Service config controller
    srvCfg     = None
    ## Running workloads processes
    workloads  = []

    ## Name of node sections already deployed
    deployedNodeSections = []

    def __init__(self, cfgFile, verbose, debug):
        self.numPuts    = 10
        self.numGets    = 5000
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
                self.deployedNodeSections.append(section)
                return 0
        # No more nodes to deploy
        return -1

    ## Remove node from deployed list
    #
    def removeDeployedNode(self, rmSection):
        assert(self.deployedNodeSections.count(rmSection) == 1)
        index = self.deployedNodeSections.index(rmSection)
        self.deployedNodeSections.remove(rmSection)

    ## Shutsdown a deployed node
    #
    # Chose next node to shutdown. A shutdown cleanly
    # removes a node from the cluster, but does not undeploy
    # it (i.e., process is still running)
    # TODO: Only works for SMs today
    def nextNodeShutdown(self, service=None):
        # Find non-OM node to undeploy
        nodeSections = self.srvCfg.getNodeSections()
        for section in nodeSections:
            if self.isNodeDeployed(section) == True:
                if self.srvCfg.isNodeSectionOm(section) == False:
                    result = self.srvCfg.shutdownSection(section, service)
                    result = 0
                    if result != 0:
                        print "Failed to undeploy %s" % (section)
                        return result
                    # TODO: We leave the node in the deployed list since
                    # we need to clean up the process and cant handle adding
                    # it back anyways
                    self.removeDeployedNode(section)
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
    @staticmethod
    def s3Workload(bucketName,
                   numConns,
                   numBuckets,
                   numObjs,
                   numPuts,
                   numGets,
                   maxObjSize):
        print "Started s3 workload"

        # Local cache of what was put, used to verify gets
        putData = {}
        # Local random object data generator
        dataGen = ServiceWkld.GenObjectData(maxObjSize)
        # workload handler
        s3wkld  = ServiceWkld.S3Wkld("localhost", 8000,
                                     False, False,
                                     numConns,
                                     numBuckets,
                                     numObjs)

        s3wkld.openConns()

        bucket = s3wkld.createBucket(0, bucketName)
        for i in range(0, numPuts):
            objName = "object%d" % (i)
            data    = dataGen.genRandData()
            result  = s3wkld.putObject(0, bucket, objName, data)
            if result != True:
                print "Failed to put object %s" % (objName)
                assert(0)
            else:
                print "Put object %s of size %d" % (objName, len(data))
                putData[objName] = data

        keys = putData.keys()
        for i in range(0, numGets):
            # Select a random object to read
            objName = keys[random.randrange(0, len(keys))]
            data    = s3wkld.getObject(0, bucket, objName)
            if data != putData[objName]:
                print "Failed to get correct data for object %s" % (objName)
                assert(0)
            if (i % 100) == 0:
                print "Completed %d object gets()" % (i)
            

        s3wkld.closeConns()
        print "Finished s3 workload"

    ## Incremental S3 workload to run in background
    #
    # This workload assumes an initial, possibly larger,
    # workload has already populated the system.
    def incS3Workload(self):
        print "Started incremental s3 workload"

    ## Starts an background s3 workload
    #
    def startS3Workload(self, bucketName):
        s3process = multiprocessing.Process(target=self.s3Workload,
                                            name="s3 workload process %s" % (bucketName),
                                            args=(bucketName, self.numConns,
                                                  self.numBuckets, self.numObjs,
                                                  self.numPuts, self.numGets,
                                                  self.maxObjSize))
        print "Forking %s workload process" % (s3process.name)
        s3process.start()
        assert(s3process.is_alive() == True)
        self.workloads.append(s3process)

    ## Waits for workloads to finish
    def joinWorkloads(self):
        for workload in self.workloads:
            workload.join()
            if workload.is_alive() == True:
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

    # Start background workloads
    mnt.startS3Workload("volume4")
    mnt.startS3Workload("volume5")

    # Deploy another node
    result = 0
    while result != -1:
        time.sleep(4)
        result = mnt.nextNodeDeploy("SM")
        if (result != 0) and (result != -1):
            print "Failed to deploy next node"
            sys.exit()
        else:
            print "Deployed next node"

    # Shutdown node
    result = 0
    while result != -1:
        time.sleep(4)
        result = mnt.nextNodeShutdown("SM")
        if (result != 0) and (result != -1):
            print "Failed to undeploy next node"
            sys.exit()
        else:
            print "Undeployed next node"

    # Wait for all workloads to finish
    mnt.joinWorkloads()

    # Undeploy the entire cluster
    mnt.fullUndeploy()

    
