#
# Copyright 2014 Formation Data Systems, Inc.
#
import ConfigParser
import re
import pdb
import traceback
import time
import ServiceMgr

##
# Defines how to bring up a cluster
#
class TestBringUp():
    #
    # Config and cluster elements
    #
    cfgFile   = None
    nodeIds   = []
    nodeIdsEnable = []
    clientIds = []
    volumeIds = []
    policyIds = []

    #
    # Debug params
    #
    verbose = False
    debug   = False

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

    def __init__(self, _cfg_file, verbose, debug):
        self.cfgFile = _cfg_file
        self.verbose = verbose
        self.debug   = debug

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
        root      = None
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
            elif key == "fds_root":
                root = value
            else:
                print "Unknown item %s, %s in %s" % (key, value, name)

        # Add client to the inventory
        ident = self.deployer.clientService.addClient(name, clientIp, clientBlk, clientLog, root)
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
        enable  = True
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
            elif key == "enable":
                enable = value
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
                                                         verbose=self.verbose,
                                                         debug=self.debug)

        # Add node to the inventory
        ident = self.deployer.nodeService.addNode(name, isOm, ip, cp, dp, confP,
                                                  mp, omCtrlP, log, root)
        # Keep a record of the node's ID
        self.nodeIds.append(ident)
        self.nodeIdsEnable.append(enable)

    ##
    # Parses the cfg file and populates
    # cluster info.
    #
    def loadCfg(self, _user=None, _passwd=None, _passPkey=None):
        self.config = ConfigParser.ConfigParser()
        self.config.read(self.cfgFile)

        self.sshUser = _user
        self.sshPswd = _passwd
        self.sshPkey = _passPkey

        #
        # Iterate over each section
        #
        for section in self.config.sections():
            if re.match(self.node_sec_prefix, section) != None:
                self.loadNode(section, self.config.items(section))
            elif re.match(self.src_sec_prefix, section) != None:

                self.srcPath = self.config.get(section, "path")

                if _user == None:
                    self.sshUser = self.config.get(section, "user")
                if _passwd == None:
                    self.sshPswd = self.config.get(section, "passwd")
                if self.config.has_option(section, "privatekey") == True and \
                   _passPkey == None:
                    self.sshPkey = self.config.get(section, "privatekey")
            elif re.match(self.sh_sec_prefix, section) != None:
                self.loadClient(section, self.config.items(section))
            elif re.match(self.vol_sec_prefix, section) != None:
                self.loadVol(section, self.config.items(section))
            elif re.match(self.pol_sec_prefix, section) != None:
                self.loadPolicy(section, self.config.items(section))
            else:
                print "Unknown section %s" % (section)

            if self.verbose == True:
                print "Parsed section %s with items %s" % (section,
                                                           self.config.items(section))
    def setCfgUser(self, _user):
        self.sshUser = _user

    def setCfgPasswd(self, _passwd):
        self.sshPswd = _passwd

    def setCfgSshPKey(self, _sshKey):
        self.sshPkey = _sshKey

    def getSectionField(self, section, items, field):
        for i in items:
            if i[0] == field:
                return i[1]
        return "ERROR_NOT_FOUND"

    def getCfgField(self, sectName, field):
        self.config = ConfigParser.ConfigParser()
        self.config.read(self.cfgFile)

        #
        # Iterate over each section
        #
        for section in self.config.sections():
            if sectName == section:
                if self.verbose == True:
                    print "Parsed section %s with items %s" % (section,
                                                               self.config.items(section))
                return self.getSectionField(section, self.config.items(section), field)

        return None

    ## Returns list of all node sections
    #
    def getNodeSections(self):
        sections = []
        for nodeId in self.nodeIds:
            section = self.deployer.nodeService.getNodeName(nodeId)
            sections.append(section)
        return sections

    ## Returns true if node section can run an OM
    def isNodeSectionOm(self, _section):
        return self.deployer.nodeService.isOmByName(_section)

    ## Returns true if a node section is enable
    def isNodeIdEnable(self, nodeId):
        return self.nodeIdsEnable[nodeId] == True

    ## Gets the first (usually only) node section
    # that can run an OM, none if none exists
    def getOmNodeSection(self):
        sections = self.getNodeSections()
        for section in sections:
            isOm = self.isNodeSectionOm(section)
            if isOm == True:
                return section
        return None

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
            if self.isNodeIdEnable(nodeId):
                if self.deployer.nodeService.isOm(nodeId):
                    result = self.deployer.nodeService.deployNode(nodeId)
                    if result != 0:
                        return result
                    upAlready = nodeId
                    break

        #
        # Bring up other node services
        #
        for nodeId in self.nodeIds:
            if self.isNodeIdEnable(nodeId):
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

    ## Bring up a specific section of the config file
    #
    # The section can be of any type. The section type is derived
    # from the section name passed in.
    # We're assuming that the section name matches the name of the
    # service/resource of that type
    def bringUpSection(self, section, service=None):
        if re.match(self.node_sec_prefix, section) != None:
            # Deploy all services
            if service == None:
                result = self.deployer.nodeService.deployNodeByName(section)
                if result != 0:
                    return result
            else:
                # Deploy a specific service
                result = self.deployer.nodeService.deployNodeServiceByName(section, service)
                if result != 0:
                    return result
        elif re.match(self.sh_sec_prefix, section) != None:
            result = self.deployer.clientService.deployClientByName(section)
        else:
            assert(0)

        return 0            

    ## Bring down a specific section of the config file
    #
    # The section can be of any type. The section type is derived
    # from the section name passed in.
    # We're assuming that the section name matches the name of the
    # service/resource of that type
    def bringDownSection(self, section):
        if re.match(self.node_sec_prefix, section) != None:
            result = self.deployer.nodeService.undeployNodeByName(section)
            if result != 0:
                return result
        elif re.match(self.sh_sec_prefix, section) != None:
            result = self.deployer.clientService.undeployClientByName(section)
        else:
            assert(0)
        return 0

    ## Shuts down a specific section of the config file
    #
    # A shutdown is a clean undeployment path. A shutdown
    # removes the section from the cluster but may not
    # shutdown the process
    def shutdownSection(self, section, service=None):
        # Currently only handle node/SM sections
        if re.match(self.node_sec_prefix, section) != None:
            # TODO: Don't just hard code to SM
            assert(service == "SM")
            result = self.deployer.nodeService.shutdownNodeServiceByName(section, service)
            if result != 0:
                return result
        else:
            assert(0)

        return 0
