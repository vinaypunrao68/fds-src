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
# Defines a node in the cluster
#
class StorNode():
    ipStr = "127.0.0.1"
    name = None
    controlPort = 6900
    dataPort = 7900
    migPort = 8600
    configPort = None
    omControlPort = None
    isOm = False
    logSeverity = 2
    fdsRoot = "/fds"
    omBin = "orchMgr"
    dmBin = "DataMgr"
    smBin = "StorMgr"
    logName = None
    
    def __init__(self, _name):
        self.name = _name
        self.logName = self.name

    def setOm(self, _om):
        self.isOm = _om

    def setIp(self, _ip):
        self.ipStr = _ip

    def setCp(self, _cp):
        self.controlPort = _cp

    def setDp(self, _dp):
        self.dataPort = _dp

    def setConfPort(self, _cp):
        self.configPort = _cp

    def setMigPort(self, _mp):
        self.migPort = _mp

    def setOCPort(self, _cp):
        self.omControlPort = _cp 
    
    def setLogSeverity(self, _sev):
        self.logSeverity = _sev

    def setFdsRoot(self, _root):
        self.fdsRoot = _root
        
    def getOmCmd(self):
        return "%s --fds.om.config_port=%d --fds.om.control_port=%d --fds.om.prefix=%s_ --fds.om.log_severity=%d" % (self.omBin,
                                              self.configPort,
                                              self.omControlPort,
                                              self.name,
                                              self.logSeverity)

    def getSmCmd(self):
        return "%s --fds-root=%s --fds.sm.data_port=%d --fds.sm.control_port=%d --fds.sm.migration.port=%d --fds.sm.prefix=%s_ --fds.sm.test_mode=false --fds.sm.log_severity=%d --fds.sm.logfile=sm.%s" % (self.smBin,
                                      self.fdsRoot,
                                      self.dataPort,
                                      self.controlPort,
                                      self.migPort,
                                      self.name,
                                      self.logSeverity,
                                      self.logName)

    def getDmCmd(self):
        return "%s --fds.dm.port=%d --fds.dm.cp_port=%d --fds.dm.prefix=%s_ --fds.dm.log_severity=%d  --fds.dm.logfile=dm.%s" % (self.dmBin,
                                                           self.dataPort + 1,
                                                           self.controlPort + 1,
                                                           self.name,
                                                           self.logSeverity,
                                                           self.logName)

    def getOmBin(self):
        return self.omBin

    def getSmBin(self):
        return self.smBin

    def getDmBin(self):
        return self.dmBin

    def getLogSeverity(self):
        return self.logSeverity 

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

    #
    # The default IP of the OM that each
    # node will boot off off. Each cluster
    # bring up requires at least one OM.
    #
    omIpStr    = None
    omConfPort = None
    omCtrlPort = None

    #
    # SSH login params
    #
    sshUser = None
    sshPswd = None
    sshPort = 22
    sshPkey = None

    #
    # Relative path info on remote node
    #
    srcPath      = None
    ldPath       = None
    icePath      = None
    fdsBinDir    = "Build/linux-*/bin"
    fdsIceDir    = "../Ice-3.5.0"
    fdsLdbLibDir = "../leveldb-1.12.0/"
    fdsIceLibDir = "../Ice-3.5.0/cpp/lib/"
    fdsBstLibDir = "/usr/local/lib"
    fdsLibcfgLibDir = "../libconfig-1.4.9/lib/.libs"
    fdsThriftLibDir = "../thrift-0.9.0"
    ldLibPath    = "export LD_LIBRARY_PATH=$LD_LIBRARY_PATH"
    iceHome      = "export ICE_HOME="

    deployer = None

    def __init__(self, _cfg_file):
        self.cfgFile = _cfg_file

    ##
    # Sets up cluster path variables
    #
    def setPath(self, _path):
        self.srcPath = _path
        self.fdsIceDir = self.srcPath + "/" + self.fdsIceDir
        self.fdsLdbLibDir = self.srcPath + "/" + self.fdsLdbLibDir
        self.fdsIceLibDir = self.srcPath + "/" + self.fdsIceLibDir
        self.fdsLibcfgLibDir = self.srcPath + "/" + self.fdsLibcfgLibDir
        self.fdsThriftLibDir = self.srcPath + "/" + self.fdsThriftLibDir

        self.fdsBinDir = self.srcPath + "/" + self.fdsBinDir
        self.ldLibPath = self.ldLibPath + ":" + self.fdsLdbLibDir + ":" + self.fdsIceLibDir + ":" + self.fdsBstLibDir + ":" + self.fdsLibcfgLibDir + ":" + self.fdsThriftLibDir
        self.iceHome = self.iceHome + self.srcPath + "/" + self.fdsIceDir


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
            self.omIpStr    = ip
            self.omConfPort = confP
            self.omCtrlPort = omCtrlP
            if (self.omIpStr == None) or (self.omConfPort == None) or (self.omCtrlPort == None):
                print "Error: An OM port and IP must be specified"
                return -1

            # Can create the deployer once we have all of the
            # OM & SSH info
            if self.deployer == None:
                self.deployer = ServiceMgr.ServiceDeploy(self.srcPath, self.sshUser,
                                                         self.sshPswd, self.sshPkey,
                                                         self.omIpStr, self.omConfPort,
                                                         self.omCtrlPort, verbose=verbose,
                                                         debug=debug)

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
                self.setPath(self.config.get(section, "path"))
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
    # Builds the command to bring up a node service
    #
    def buildNodeCmd(self, node, nodeType = None):
        cmd = self.ldLibPath + "; " + self.iceHome + "; " + "cd " + self.fdsBinDir + "; " + "ulimit -c unlimited;" + "./"
        if nodeType == "SM":
            cmd += node.getSmCmd()
            cmd += " --fds.sm.om_ip=%s --fds.sm.om_port=%d" % (self.omIpStr,
                                                 self.omCtrlPort)
        elif nodeType == "DM":
            cmd += node.getDmCmd()
            cmd += " --fds.dm.om_ip=%s --fds.dm.om_port=%d" % (self.omIpStr,
                                                 self.omCtrlPort)
        elif node.isOm == True:
            cmd += node.getOmCmd()
        else:
            assert(0)

        return cmd

    ##
    # Builds the command to run a remote command
    #
    def runNodeCmd(self, node, cmd, checkStr = None, ipOver = None):
        outputChecked = False
        #
        # If the ip wasn't overloaded via params
        # use the node's IP
        #
        if ipOver == None:
            ipOver = node.ipStr

        try:
            #
            # Connect to remove host
            #
            client = paramiko.SSHClient()
            client.load_system_host_keys()
            client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
            if self.sshPkey == None:
                client.connect(ipOver, self.sshPort, self.sshUser, self.sshPswd)
            else:
                key = paramiko.RSAKey.from_private_key_file(self.sshPkey)
                client.connect(ipOver, port=self.sshPort, username=self.sshUser, pkey=key)

            #
            # Run remote cmd
            #
            if verbose == True:
                print "Running remote command: %s" % (cmd)
            if debug == True:
                pdb.set_trace()
            stdin, stdout, stderr = client.exec_command(cmd)

            if checkStr != None:
                line = stderr.readline()
                print line
                while line != "":
                    if verbose == True:
                        print line
                    if re.search(checkStr, line):
                        outputChecked = True
                        break
                    line = stderr.readline()

            time.sleep(10)
            stdin.close()
            stdout.close()
            stderr.close()

            client.close()

        except Exception, e:
            print "*** Caught exception %s: %s" % (e.__class__, e)
            traceback.print_exc()
            try:
                client.close()
            except:
                pass
            return False

        if checkStr != None:
            return outputChecked
        return True

    ##
    # Bring down all of the services associated
    # with this node
    #
    def nodeBringDown(self, node):
        #
        # Forcefully brings down the processes
        #
        cmd = "pkill -9 "

        #
        # Bring down the SM and DM services first
        #
        smCmd = cmd + node.getSmBin()
        result = self.runNodeCmd(node, smCmd)
        if result == True:
            print "Brought down SM on %s..." % (node.ipStr)
        else:
            print "Failed to bring down SM"
            return -1

        dmCmd = cmd + node.getDmBin()
        result = self.runNodeCmd(node, dmCmd)
        if result == True:
            print "Brought down DM on %s..." % (node.ipStr)
        else:
            print "Failed to bring down DM"
            return -1

        #
        # Bring down OM service if needed
        #
        if node.isOm == True:
            omCmd = cmd + node.getOmBin()
            result = self.runNodeCmd(node, omCmd)
            if result == True:
                print "Brought down OM on %s..." % (node.ipStr)
            else:
                print "Failed to bring down OM"
                return -1



        return 0

    ##
    # Bring ups all of the services associated with
    # this node.
    #
    def nodeBringUp(self, node):
        try:
            cmd = None

            #
            # Bring up OM is this node runs OM
            #
            if node.isOm == True:
                cmd = self.buildNodeCmd(node)
                started = self.runNodeCmd(node, cmd, "Starting the server")
                if started == True:
                    print "Server OM running on %s..." % (node.ipStr)
                else:
                    print "Failed to start OM server..."
                    return -1

            #
            # Bring up SM
            #
            cmd = self.buildNodeCmd(node, "SM")
            started = self.runNodeCmd(node, cmd, "Starting the server")            
            if started == True:
                print "Server SM running on %s..." % (node.ipStr)
            else:
                print "Failed to start SM server..."
                return -1

            #
            # Bring up DM
            #
            cmd = cmd = self.buildNodeCmd(node, "DM")
            started = self.runNodeCmd(node, cmd, "Starting the server")            
            if started == True:
                print "Server DM running on %s..." % (node.ipStr)
            else:
                print "Failed to start DM server..."
                return -1

        except Exception, e:
            print "*** Caught exception %s: %s" % (e.__class__, e)
            traceback.print_exc()
            try:
                client.close()
            except:
                pass
            return -1

        time.sleep(15)
        return 0

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
                #result = self.nodeBringUp(node)
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
                #result = self.nodeBringUp(node)
                result = self.deployer.nodeService.deployNode(nodeId)
                if result != 0:
                    return result
        return 0

    ##
    # Bring down services on all cluster nodes
    #
    def bringDownNodes(self):
        for nodeId in self.nodeIds:
            #result = self.nodeBringDown(node)
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
