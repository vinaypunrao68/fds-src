#!/usr/bin/python
#
# Copyright 2013 Formation Data Systems, Inc.
#
import optparse
import paramiko
import ConfigParser
import re
import pdb
import traceback
import time
import ServiceMgr

verbose = False
debug = False

##
# Defines how to bring up a cluster
#
class TestBringUp():
    #
    # Config and cluster elements
    #
    cfgFile   = None
    nodeIds   = []
    clientIds = []
    volumeIds = []
    policyIds = []

    #
    # Prefixes for parsing the cfg file
    #
    node_sec_prefix = "node"
    test_sec_prefix = "test"
    sthv_sec_prefix = "sthv"
    src_sec_prefix  = "source"
    sh_sec_prefix   = "sh"
    vol_sec_prefix  = "volume"
    pol_sec_prefix  = "policy"

    # SSH login params that
    # are needed to be passed to
    # deployer
    sshUser = None
    sshPswd = None
    sshPort = 22
    sshPkey = None
    # Src path needed to be passed
    # to deployer
    srcPath = None

    deployer = None

    def __init__(self, _cfg_file):
        self.cfgFile = _cfg_file

    ##
    # Loads a volume from items
    # in the cfg file
    #
    def loadPolicy(self, name, items):
        polId    = None
        iopsMin  = None
        iopsMax  = None
        priority = None
        for item in items:
            key   = item[0]
            value = item[1]
            if key == "id":
                polId = int(value)
            elif key == "iops_min":
                iopsMin = int(value)
            elif key == "iops_max":
                iopsMax = int(value)
            elif key == "priority":
                priority = int(value)
            else:
                print "Unknown item %s, %s in %s" % (key, value, name)

        # Add policy to the inventory
        ident = self.deployer.volService.addPolicy(name, polId, iopsMin,
                                                   iopsMax, priority)
        # Keep a record of the policy's ID
        self.policyIds.append(ident)

    ##
    # Loads a volume from items
    # in the cfg file
    #
    def loadVol(self, name, items):
        volId     = None
        volSize   = None
        volPol    = None
        volClient = None
        volConn   = None
        for item in items:
            key   = item[0]
            value = item[1]
            if key == "id":
                volId = int(value)
            elif key == "size":
                volSize = int(value)
            elif key == "policy":
                volPol = int(value)
            elif key == "client":
                volClient = value
            elif key == "access":
                volConn = value
            else:
                print "Unknown item %s, %s in %s" % (key, value, name)

        # Add volume to the inventory
        ident = self.deployer.volService.addVol(name, volId, volSize,
                                                volPol, volClient, volConn)
        # Keep a record of the volume's ID
        self.volumeIds.append(ident)

    ##
    # Loads a client from items
    # in the cfg file
    #
    def loadClient(self, name, items):
        clientIp  = None
        clientBlk = None
        clientLog = None
        for item in items:
            key   = item[0]
            value = item[1]
            if key == "ip":
                clientIp = value
            elif key == "blk":
                if value == "true":
                    clientBlk = True
                else:
                    clientBlk = False
            elif key == "log_severity":
                clientLog = int(value)
            else:
                print "Unknown item %s, %s in %s" % (key, value, name)

        # Add client to the inventory
        ident = self.deployer.clientService.addClient(name, clientIp, clientBlk, clientLog)
        # Keep a record of the client's ID
        self.clientIds.append(ident)

    ##
    # Loads a node from items
    # in the cfg file
    #
    def loadNode(self, name, items):
        isOm    = None
        ip      = None
        cp      = None
        dp      = None
        confP   = None
        mp      = None
        omCtrlP = None
        log     = None
        root    = None
        for item in items:
            key = item[0]
            value = item[1]
            if key == "om":
                if value == "true":
                    isOm = True
                else:
                    isOm = False
            elif key == "ip":
                ip = value
            elif key == "control_port":
                cp = int(value)
            elif key == "data_port":
                dp = int(value)
            elif key == "config_port":
                confP = int(value)
            elif key == "migration_port":
                mp = int(value)
            elif key == "om_control_port":
                omCtrlP = int(value)
            elif key == "log_severity":
                log = int(value)
            elif key == "fds_root":
                root = value
            else:
                print "Unknown item %s, %s in %s" % (key, value, name)
        
        if isOm == True:
            if (ip == None) or (confP == None) or (omCtrlP == None):
                print "Error: An OM port and IP must be specified"
                return -1

            # Can create the deployer once we have all of the
            # OM & SSH info
            if self.deployer == None:
                self.deployer = ServiceMgr.ServiceDeploy(self.srcPath, self.sshUser,
                                                         self.sshPswd, self.sshPkey,
                                                         ip, confP, omCtrlP,
                                                         verbose=verbose, debug=debug)

        # Add node to the inventory
        ident = self.deployer.nodeService.addNode(name, isOm, ip, cp, dp, confP,
                                                  mp, omCtrlP, log, root)
        # Keep a record of the node's ID
        self.nodeIds.append(ident)

    ##
    # Parses the cfg file and populates
    # cluster info.
    #
    def loadCfg(self):
        self.config = ConfigParser.ConfigParser()
        self.config.read(self.cfgFile)

        #
        # Iterate over each section
        #
        for section in self.config.sections():
            if re.match(self.node_sec_prefix, section) != None:
                self.loadNode(section, self.config.items(section))
            elif re.match(self.src_sec_prefix, section) != None:
                self.srcPath = self.config.get(section, "path")
                self.sshUser = self.config.get(section, "user")
                self.sshPswd = self.config.get(section, "passwd")
                if self.config.has_option(section, "privatekey") == True:
                    self.sshPkey = self.config.get(section, "privatekey")
            elif re.match(self.sh_sec_prefix, section) != None:
                self.loadClient(section, self.config.items(section))
            elif re.match(self.vol_sec_prefix, section) != None:
                self.loadVol(section, self.config.items(section))
            elif re.match(self.pol_sec_prefix, section) != None:
                self.loadPolicy(section, self.config.items(section))
            else:
                print "Unknown section %s" % (section)

            if verbose == True:
                print "Parsed section %s with items %s" % (section,
                                                           self.config.items(section))

    ##
    # Deploys all policies in the inventory
    #
    def bringUpPols(self):
        for policyId in self.policyIds:
           result = self.deployer.volService.deployPolicy(policyId)
           if result != 0:
               return result
        return 0

    ##
    # Undeploys all policies in the invetory
    #
    def bringDownPols(self):
        for policyId in self.policyIds:
           result = self.deployer.volService.undeployPolicy(policyId)
           if result != 0:
               return result
        return 0

    ##
    # Deploys all volumes in the inventory
    #
    def bringUpVols(self):
        for volumeId in self.volumeIds:
            result =  self.deployer.volService.deployVol(volumeId)
            if result != 0:
                return result        
        return 0

    ##
    # Bring up all services on all cluster nodes
    #
    def bringUpNodes(self):
        #
        # Bring up a node with OM service first
        #
        for nodeId in self.nodeIds:
            if self.deployer.nodeService.isOM(nodeId):
                result = self.deployer.nodeService.deployNode(nodeId)
                if result != 0:
                    return result
                upAlready = nodeId
                break

        #
        # Bring up other node services
        #
        for nodeId in self.nodeIds:
            if nodeId != upAlready:
                result = self.deployer.nodeService.deployNode(nodeId)
                if result != 0:
                    return result
        return 0

    ##
    # Bring down services on all cluster nodes
    #
    def bringDownNodes(self):
        for nodeId in self.nodeIds:
            result = self.deployer.nodeService.undeployNode(nodeId)
            if result != 0:
                return result
        return 0

    ##
    # Bring up services on all cluster clients
    #
    def bringUpClients(self):
        for clientId in self.clientIds:
            result = self.deployer.clientService.deployClient(clientId)
            if result != 0:
                return result
        return 0

    ##
    # Bring down services on all cluster clients
    #
    def bringDownClients(self):
        for clientId in self.clientIds:
            result = self.deployer.clientService.undeployClient(clientId)
            if result != 0:
                return result
        return 0

if __name__ == '__main__':
    #
    # Parse command line options
    #
    parser = optparse.OptionParser("usage: %prog [options]")
    parser.add_option("-f", "--file", dest="config_file",
                      help="configuration file", metavar="FILE")
    parser.add_option("-v", "--verbose", action="store_true",
                      dest="verbose", help="enable verbosity")
    parser.add_option("-b", "--debug", action="store_true",
                      dest="debug", help="enable debug breaks")
    parser.add_option("-u", "--up", action="store_true",
                      dest="up", help="bring up cluster")
    parser.add_option("-d", "--down", action="store_true",
                      dest="down", help="bring down cluster")
    (options, args) = parser.parse_args()
    cfgFile = options.config_file
    verbose = options.verbose
    debug = options.debug
    up = options.up
    down = options.down

    #
    # Load the configuration files
    #
    bu = TestBringUp(cfgFile)
    bu.loadCfg()

    #
    # Bring up the services
    #
    if up == True:
        result = bu.bringUpNodes()
        if result != 0:
            print "Failed to bring up all nodes"
        else :
            print "Brought up all nodes"
        result = bu.bringUpClients()
        if result != 0:
            print "Failed to bring up all clients"
        else :
            print "Brought up all clients"
        result = bu.bringUpPols()
        if result != 0:
            print "Failed to bring up all policies"
        else :
            print "Brought up all policies"
        result = bu.bringUpVols()
        if result != 0:
            print "Failed to bring up all volumes"
        else :
            print "Brought up all volumes"

    if down == True:
        result = bu.bringDownClients()
        if result != 0:
            print "Failed to bring down all clients"
        else :
            print "Brought down all clients"
        result = bu.bringDownNodes()
        if result != 0:
            print "Failed to bring down all nodes"
        else:
            print "Brought down all nodes"
