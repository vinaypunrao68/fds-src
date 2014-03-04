#
# Copyright 2014 Formation Data Systems, Inc.
#
import paramiko
import pdb
import traceback
import time
import re

## Defines a volume in the cluster
#
class Volume():
    ## Name of the volume
    name   = None
    ## UUID of the volume
    volId  = None
    ## Max size of the volume
    size   = None
    ## Associated policy
    policy = None
    ## Client/AM to attach to
    client = None
    ## Connector interface
    connector = None
    ## Binary to run/create the volume
    binary = "fdscli"

    ## Class constructor
    def __init__(self,
                 _name,
                 _id,
                 _size,
                 _policy,
                 _client,
                 _connector):
        self.setName(_name)
        self.setId(_id)
        self.setSize(_size)
        self.setPolicy(_policy)
        self.setClient(_client)
        self.setConnector(_connector)

    def setName(self, _name):
        self.name = _name

    def setId(self, _id):
        self.volId = _id

    def setSize(self, _size):
        self.size = _size

    def setPolicy(self, _polId):
        self.policy = _polId

    def setClient(self, _clientName):
        self.client = _clientName

    def setConnector(self, _connector):
        self.connector = _connector

    def getCrtCmd(self):
        return "%s --volume-create %s -i %d -s %d -p %d -y %s" % (self.binary,
                                                                  self.name,
                                                                  self.volId,
                                                                  self.size,
                                                                  self.policy,
                                                                  self.connector)

    def getAttCmd(self):
        # The client name is hard coded for now because
        # it is hard coded in the SH. As a result, the
        # local client variable is ignored.
        return "%s --volume-attach %s -i %d -n localhost-%s" % (self.binary,
                                                                self.name,
                                                                self.volId,
                                                                self.client)

## Defines a volume policy in the cluster
#
class Policy():
    ## Name of the policy
    name     = None
    ## UUID of the policy
    polId    = None
    ## Min iops
    iopsmin  = None
    ## Max ips
    iopsmax  = None
    ## Relative priority
    priority = None
    ## Binary to create/delete policies
    binary   = "fdscli"

    def __init__(self,
                 _name,
                 _id,
                 _min,
                 _max,
                 _prio):
        self.setName(_name)
        self.setId(_id)
        self.setIopsMin(_min)
        self.setIopsMax(_max)
        self.setPriority(_prio)

    def setName(self, _name):
        self.name = _name

    def setId(self, _id):
        self.polId = _id

    def setIopsMin(self, _min):
        self.iopsmin = _min

    def setIopsMax(self, _max):
        self.iopsmax = _max

    def setPriority(self, _prio):
        self.priority = _prio

    def getCrtCmd(self):
        return "%s --policy-create %s -p %d -g %d -m %d -r %d" % (self.binary,
                                                                  self.name,
                                                                  self.polId,
                                                                  self.iopsmin,
                                                                  self.iopsmax,
                                                                  self.priority)

    def getDelCmd(self):
        return "%s --policy-delete %s -p %d" % (self.binary,
                                                self.name,
                                                self.polId)

## Defines a client in the cluster
#
class Client():
    ## IP of the client
    ipStr = None
    ## Name of the client
    name  = None
    ## Id of the client (internal to module)
    clientId = None
    ## Access method
    isBlk = False
    ## Client logging level
    logSeverity = 2
    ## Fds root for client to use
    root = None
    ## Binary of client AM
    amBin  = "AMAgent"
    ## Binary of client block
    # We can remove this when ubd/AM combine
    ubdBin = "ubd"
    ## Dir location of blk kernel mod
    blkDir = "fds_client/blk_dev"
    ## Name of blk kernel mod
    blkMod = "fbd.ko"

    def __init__(self,
                 _name,
                 _id,
                 _ip,
                 _blk,
                 _log,
                 _root):
        assert(_name != None and _id != None and _ip != None or _root != None)
        self.setName(_name)
        self.setId(_id)
        self.setIp(_ip)
        self.setBlk(_blk)
        self.setLogSeverity(_log)
        self.setRoot(_root)

    def setName(self, _name):
        self.name = _name

    def setId(self, _id):
        self.clientId  = _id

    def setIp(self, _ip):
        self.ipStr = _ip

    def setBlk(self, _blk):
        self.isBlk = _blk

    def setLogSeverity(self, _sev):
        self.logSeverity = _sev

    def setRoot(self, _root):
        self.root = _root

    def getBlkCmd(self, srcPath):
        return "insmod %s/%s" % (srcPath + "/" + self.blkDir,
                                 self.blkMod)

    def getRmCmd(self, srcPath):
        return "rmmod %s/%s" % (srcPath + "/" + self.blkDir,
                                self.blkMod)

    def getUbdCmd(self):
        return self.ubdBin

    def getAmCmd(self):
        return self.amBin

    def getLogSeverity(self):
        return self.logSeverity 

## Manages config info for deployment of services
#
# This is a service class that
# provides member functions to deploy or
# undeploy remote services
class DeployConfig():
    #
    # Fields for setting path and env variables
    #
    srcPath         = None
    ldPath          = None
    fdsLdbLibDir    = "../leveldb-1.12.0/"
    fdsLibcfgLibDir = "../libconfig-1.4.9/lib/.libs"
    fdsThriftLibDir = "../thrift-0.9.0"
    fdsBinDir       = "Build/linux-*/bin"
    fdsBstLibDir    = "/usr/local/lib"
    fdsJavaLibDir   = "/usr/lib/jvm/java-8-oracle/jre/lib/amd64"
    fdsJavaSrvDir   = "/usr/lib/jvm/java-8-oracle/jre/lib/amd64/server"
    ldLibPath       = "export LD_LIBRARY_PATH=$LD_LIBRARY_PATH"

    #
    # SSH login params
    #
    sshUser = None
    sshPswd = None
    sshPort = 22
    sshPkey = None

    #
    # The default IP of the OM that each
    # node will boot off off. Each cluster
    # requires at least one OM.
    #
    omIpStr    = None
    omConfPort = None
    omCtrlPort = None

    #
    # Local configs
    #
    verbose    = None
    debug      = None

    def __init__(self, _path, _user, _pswd, _pkey,
                 _omIp, _omConf, _omCtrl, _verbose, _debug):
        self.setPath(_path)
        self.setSshUser(_user)
        self.setSshPswd(_pswd)
        self.setSshPkey(_pkey)
        self.setOmIpStr(_omIp)
        self.setOmConfPort(_omConf)
        self.setOmCtrlPort(_omCtrl)

        self.verbose = _verbose

    def setPath(self, _path):
        self.srcPath = _path

        # Set the source paths to be absolute based on our
        # given source path
        self.fdsLdbLibDir    = self.srcPath + "/" + self.fdsLdbLibDir
        self.fdsLibcfgLibDir = self.srcPath + "/" + self.fdsLibcfgLibDir
        self.fdsThriftLibDir = self.srcPath + "/" + self.fdsThriftLibDir
        self.fdsBinDir       = self.srcPath + "/" + self.fdsBinDir

        # Create the full LD library path
        self.ldLibPath = self.ldLibPath + ":" + self.fdsLdbLibDir + ":" + self.fdsBstLibDir + ":" + self.fdsLibcfgLibDir + ":" + self.fdsThriftLibDir + ":" + self.fdsJavaLibDir + ":" + self.fdsJavaSrvDir

    def setSshUser(self, _user):
        self.sshUser = _user

    def setSshPswd(self, _pswd):
        self.sshPswd = _pswd

    def setSshPkey(self, _pkey):
        self.sshPkey = _pkey

    def setOmIpStr(self, _ipStr):
        self.omIpStr = _ipStr

    def setOmConfPort(self, _confPort):
        self.omConfPort = _confPort

    def setOmCtrlPort(self, _ctrlPort):
        self.omCtrlPort = _ctrlPort

    def getSrcPath(self):
        return self.srcPath

    def getOmIpStr(self):
        return self.omIpStr

    def getOmConfPort(self):
        return self.omConfPort

    def getOmCtrlPort(self):
        return self.omCtrlPort

    ## Runs a command on a remote node
    #
    # If checkStr is passed in, the command call will be sync
    # and will complete when the string is found in the processes
    # stderr output
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
            # Prefix command with env and lib goodies
            #
            cmd  = self.ldLibPath + "; " + " cd " + self.fdsBinDir + "; " + "ulimit -c unlimited; " + cmd

            #
            # Run remote cmd
            #
            if self.verbose == True:
                print "Running remote command: %s" % (cmd)
            if self.debug == True:
                pdb.set_trace()
            stdin, stdout, stderr = client.exec_command(cmd)

            if checkStr != None:
                line = stderr.readline()
                if self.verbose == True:
                    print line
                while line != "":
                    if self.verbose == True:
                        print line
                    if re.search(checkStr, line):
                        outputChecked = True
                        break
                    line = stderr.readline()

            time.sleep(2)
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
# Service to deploy/undeploy volumes
class VolService():
    ## Internal array of volumes
    volumes  = []
    ## Internal array of volume policies
    policies = []
    ## Deployer to deploy/undeploy vols
    deployer = None

    def __init__(self, _deploy):
        self.deployer = _deploy

    ## Adds a volume to the inventory
    def addVol(self,
               _name,
               _id,
               _size,
               _policy,
               _client,
               _connector):
        volume = Volume(_name,
                        _id,
                        _size,
                        _policy,
                        _client,
                        _connector)
        self.volumes.append(volume)
        return volume.volId

    def addPolicy(self,
                  _name,
                  _id,
                  _min,
                  _max,
                  _prio):
        policy = Policy(_name,
                        _id,
                        _min,
                        _max,
                        _prio)
        self.policies.append(policy)
        return policy.polId

    ##
    # Builds the command to create a volume
    #
    def buildVolCrtCmd(self, volume):
        cmd = "./" + volume.getCrtCmd() + " --om_ip=" + self.deployer.getOmIpStr() + \
            " --om_port=" + str(self.deployer.getOmConfPort())
        return cmd

    ##
    # Builds the command to attach vol to a client
    #
    def buildVolAttCmd(self, volume):
        cmd = "./" + volume.getAttCmd() + " --om_ip=" + self.deployer.getOmIpStr() + \
            " --om_port=" + str(self.deployer.getOmConfPort())
        return cmd

    ## Deploys a volume from the inventory
    #
    # Deployment includes creating the volume
    # and attaching it to a specific client
    def deployVol(self, _id):
        # Locate vol in inventory
        for volume in self.volumes:
            if volume.volId == _id:
                # Create vol with OM
                cmd = self.buildVolCrtCmd(volume)
                result  = self.deployer.runNodeCmd(volume, cmd, ipOver=self.deployer.getOmIpStr())
                if result == True:
                    print "Created volume %d with OM on %s..." % (volume.volId,
                                                                  self.deployer.getOmIpStr())
                else:
                    print "Failed to create volume %d" % (volume.volId)
                    return -1

                # Sleep to let the volume get broadcast
                # around the cluster
                time.sleep(1)

                # Tell OM to attach the volume to a client
                cmd = self.buildVolAttCmd(volume)
                result  = self.deployer.runNodeCmd(volume, cmd, ipOver=self.deployer.getOmIpStr())
                if result == True:
                    print "Attached volume %d to AM %s with OM on %s..." % (volume.volId,
                                                                            volume.client,
                                                                            self.deployer.getOmIpStr())
                else:
                    print "Failed to create volume %d" % (volume.volId)
                    return -1

                # Sleep to let the volume attach
                time.sleep(1)
        return 0

    ## Undeploys a volume from the inventory
    #
    # Undeployment includes detaching the volume
    # and deleting it from OM
    def undeployVol(self, _id):
        # TODO: Implement me.
        return 0

    ##
    # Builds the command to create a policy
    #
    def buildPolCrtCmd(self, policy):
        cmd = "./" + policy.getCrtCmd() + " --om_ip=" + self.deployer.getOmIpStr() + \
            " --om_port=" + str(self.deployer.getOmConfPort())
        return cmd

    ##
    # Builds the command to delete a policy
    #
    def buildPolDelCmd(self, policy):
        cmd = "./" + policy.getDelCmd() + " --om_ip=" + self.deployer.getOmIpStr() + \
            " --om_port=" + str(self.deployer.getOmConfPort())
        return cmd

    ##
    # Create policy in the cluster
    #
    def deployPolicy(self, _id):
        # Locate policy in the inventory
        for policy in self.policies:
            if policy.polId == _id:
                # Create policy with OM
                cmd = self.buildPolCrtCmd(policy)
                result = self.deployer.runNodeCmd(policy, cmd, ipOver=self.deployer.getOmIpStr())
                if result == True:
                    print "Created policy %d with OM on %s..." % (policy.polId,
                                                                  self.deployer.getOmIpStr())
                else:
                    print "Failed to create policy %d" % (policy.polId)
                    return -1
        return 0


    ##
    # Delete policy from the cluster
    #
    def undeployPolicy(self, _id):
        # Locate policy in the inventory
        for policy in self.policies:
            if policy.polId == _id:
                # Remove policy from OM
                cmd = self.buildPolDelCmd(policy)
                result = self.deployer.runNodeCmd(policy, cmd, ipOver=self.deployer.getOmIpStr())
                if result == True:
                    print "Deleted policy %d with OM on %s..." % (policy.polId,
                                                                  self.deployer.getOmIpStr())
                else:
                    print "Failed to delete policy %d" % (policy.polId)
                return -1
        return 0

##
# Service to deploy/undeploy clients
class ClientService():
    ## Internal array of clients
    clients  = []
    ## Deployer to deploy/undeploy clients
    deployer = None

    def __init__(self, _deploy):
        self.deployer = _deploy

    ## Adds a client to the inventory
    def addClient(self,
                  _name,
                  _ip,
                  _blk,
                  _log,
                  _root):
        # Use current index as ID
        ident = len(self.clients)
        client = Client(_name,
                        ident,
                        _ip,
                        _blk,
                        _log,
                        _root)
        self.clients.append(client)
        return client.clientId

    ##
    # Builds the command to insert a kernel module
    #
    def buildModCmd(self, client):
        cmd = client.getBlkCmd(self.deployer.getSrcPath())
        return cmd

    ##
    # Builds the command to start SH UBD service
    #
    def buildUbdCmd(self, client):
        cmd = "./" + client.getUbdCmd() + " --om_ip=" + self.deployer.getOmIpStr() + \
            " --om_port=" + str(self.deployer.getOmCtrlPort()) + " --node_name=localhost-" + client.name + \
            " --log-severity=" + str(client.getLogSeverity())
        return cmd

    ##
    # Builds the command to start SH AM service
    #
    def buildAmCmd(self, client):
        cmd = "./" + client.getAmCmd() + " --fds-root=" + client.root + \
            " --om_ip=" + self.deployer.getOmIpStr() + \
            " --om_port=" + str(self.deployer.getOmCtrlPort()) + " --node_name=localhost-" + client.name + \
            " --log-severity=" + str(client.getLogSeverity())
        return cmd

    ## Returns client object by ID
    def getClient(self, _id):        
        for client in self.clients:
            if client.clientId == _id:
                return client
        return None

    ## Returns client ID corresponding to name
    def getClientByName(self, _name):
        for client in self.clients:
            if client.name == _name:
                return client.clientId
        return None

    ## Deploys a client from the inventory
    #
    def deployClient(self, _id):
        # Locate client in inventory
        client = self.getClient(_id)
        if client == None:
            return -1

        cmd = None
        #
        # Run remote cmd based on client type
        #
        if client.isBlk == True:
            #
            # Start blktap kernel module
            #
            cmd = self.buildModCmd(client)
            started = self.deployer.runNodeCmd(client, cmd)
            if started == True:
                print "Client blk module running on %s..." % (client.ipStr)
            else:
                print "Failed to start blk module..."
                return -1

            #
            # Start UBD user space process
            #
            cmd = self.buildAmCmd(client)
            started = self.deployer.runNodeCmd(client, cmd, "Starting the server")
            if started == True:
                print "Client UBD running on %s..." % (client.ipStr)
            else:
                print "Failed to start UBD..."
                return -1
        else:
            #
            # Start AM user space process
            #
            cmd = self.buildAmCmd(client)
            started = self.deployer.runNodeCmd(client, cmd, "Starting the server")
            if started == True:
                print "Client AM running on %s..." % (client.ipStr)
            else:
                        print "Failed to start AM..."
                        return -1

        # Sleep to let client initialize
        time.sleep(3)
        return 0

    ## Deploys a client based on name
    #
    def deployClientByName(self, _name):
        clientId = self.getClientByName(_name)
        return self.deployClient(clientId)

    ## Undeploys a client from the inventory
    #
    def undeployClient(self, _id):
        # Locate client in inventory
        client = self.getClient(_id)
        if client == None:
            return -1

        if client.isBlk == True:
            #
            # Bring down UBD user space process
            #
            ubdCmd = "pkill -9 " + client.getUbdCmd()
            result = self.deployer.runNodeCmd(client, ubdCmd)
            if result == True:
                print "Brought down UBD on %s..." % (client.ipStr)
            else:
                print "Failed to bring down UBD"
                return -1

            #
            # Remove blktap kernel module
            #
            rmCmd  = client.getRmCmd(self.deployer.getSrcPath())
            result = self.deployer.runNodeCmd(client, rmCmd)
            if result == True:
                print "Removed kernel module on %s..." % (client.ipStr)
            else:
                print "Failed to remove kernel module"
                return -1
        else:
            #
            # Bring down AM user space process
            #
            amCmd = "pkill -9 " + client.getAmCmd()
            result = self.deployer.runNodeCmd(client, amCmd)
            if result == True:
                print "Brought down AM on %s..." % (client.ipStr)
            else:
                print "Failed to bring down AM"
                return -1
        return 0

    ## Undeploys a client based on name
    #
    def undeployClientByName(self, _name):
        clientId = self.getClientByName(_name)
        return self.undeployClient(clientId)

## Defines a node in the cluster
#
# A node consists of both a SM & DM service
# and possibly a OM service
class Node():
    ## IP of the node
    ipStr         = None
    ## Name of the node
    # Service names will be derived
    # from this name
    name          = None
    ## ID of the node
    # Currently local to this inventory
    nodeId        = None
    ## Base control port
    # Service control ports will be
    # derived from this port
    controlPort   = None
    ## Base data port
    # Service data ports will be
    # derived from this port
    dataPort      = None
    ## Migration port
    migPort       = None
    ## OM's config port
    configPort    = None
    ## OM's control port
    omControlPort = None
    ## Whether this node runs an OM
    isOm          = None
    ## Log level for services
    logSeverity   = 2
    ## Root directory to use
    fdsRoot       = None
    omBin         = "orchMgr"
    dmBin         = "DataMgr"
    smBin         = "StorMgr"
    logName       = None
    shutdownBin   = "fdscli"
    
    def __init__(self,
                 _name,
                 _id,
                 _om,
                 _ip,
                 _ctrlp,
                 _dp,
                 _confp,
                 _mp,
                 _omCtrlP,
                 _log,
                 _root):
        self.setName(_name)
        self.setId(_id)
        self.setOm(_om)
        self.setIp(_ip)
        self.setCp(_ctrlp)
        self.setDp(_dp)
        self.setConfPort(_confp)
        self.setMigPort(_mp)
        self.setOmCPort(_omCtrlP)
        self.setLogSeverity(_log)
        self.setFdsRoot(_root)

    def setName(self, _name):
        self.name = _name
        self.logName = self.name

    def setId(self, _id):
        self.nodeId = _id

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

    def setOmCPort(self, _cp):
        self.omControlPort = _cp 
    
    def setLogSeverity(self, _sev):
        self.logSeverity = _sev

    def setFdsRoot(self, _root):
        self.fdsRoot = _root
        
    def getOmCmd(self):
        return "%s --fds-root=%s --fds.om.config_port=%d --fds.om.control_port=%d --fds.om.prefix=%s_ --fds.om.log_severity=%d" % (self.omBin,
                                              self.fdsRoot,
                                              self.configPort,
                                              self.omControlPort,
                                              self.name,
                                              self.logSeverity)

    ## Returns the command to start a SM
    #
    # The data and control ports directly use whatever
    # the base ports are
    def getSmCmd(self):
        return "%s --fds-root=%s --fds.sm.data_port=%d --fds.sm.control_port=%d --fds.sm.migration.port=%d --fds.sm.prefix=%s_ --fds.sm.test_mode=false --fds.sm.log_severity=%d --fds.sm.logfile=sm.%s" % (self.smBin,
                                      self.fdsRoot,
                                      self.dataPort,
                                      self.controlPort,
                                      self.migPort,
                                      self.name,
                                      self.logSeverity,
                                      self.logName)

    ## Returns the command to start a DM
    #
    # The data and control ports are use +1 whatever the
    # base ports are
    def getDmCmd(self):
        return "%s --fds-root=%s --fds.dm.port=%d --fds.dm.cp_port=%d --fds.dm.prefix=%s_ --fds.dm.log_severity=%d  --fds.dm.logfile=dm.%s" % (self.dmBin,
                                                           self.fdsRoot,
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

    def getShutdownBin(self):
        return self.shutdownBin

    def getShutdownCmd(self):
        return "%s --remove-node %s_localhost-sm" % (self.getShutdownBin(), self.name)

    def getLogSeverity(self):
        return self.logSeverity

## Service to deploy/undeploy nodes
#
# Each node is capable of deploying an
# OM, SM, or DM service these servcices
# can be deployed individually or all at once
class NodeService():
    ## Internal array of nodes
    nodes = []
    ## Deployer to deploy/undeploy nodes
    deployer = None

    def __init__(self, _deploy):
        self.deployer = _deploy

    ## Adds a node to the inventory
    # If the node is not meant to run an OM,
    # the om config port does not need to be valid.
    # If the node is not meant to run an SM,
    # the sm migration port does not need to be valid.
    def addNode(self,
                 _name,
                 _om,
                 _ip,
                 _ctrlp,
                 _dp,
                 _confp,
                 _mp,
                 _omCtrlP,
                 _log,
                 _root):
        ident = len(self.nodes)
        node = Node(_name,
                    ident,
                    _om,
                    _ip,
                    _ctrlp,
                    _dp,
                    _confp,
                    _mp,
                    _omCtrlP,
                    _log,
                    _root)
        self.nodes.append(node)
        return node.nodeId

    ## Returns true if node is meant to run OM service
    def isOm(self, _id):
        node = self.getNode(_id)
        return node.isOm

    ## Returns true if node name is mean to run OM service
    def isOmByName(self, _name):
        nodeId = self.getNodeIdByName(_name)
        return self.isOm(nodeId)

    ## Returns node name
    def getNodeName(self, _id):
        node = self.getNode(_id)
        return node.name

    ## Builds the command to deploy a node service
    # 
    # The command is built depending on what type of node is
    # specified in the arguments. The node describes the box/platform/machine
    # as a whole and the type describes the service to be deployed
    def buildNodeDeployCmd(self, node, nodeType = None):
        cmd = "./"
        if nodeType == "SM":
            cmd += node.getSmCmd()
            cmd += " --fds.sm.om_ip=%s --fds.sm.om_port=%d" % (self.deployer.getOmIpStr(),
                                                               self.deployer.getOmCtrlPort())
        elif nodeType == "DM":
            cmd += node.getDmCmd()
            cmd += " --fds.dm.om_ip=%s --fds.dm.om_port=%d" % (self.deployer.getOmIpStr(),
                                                               self.deployer.getOmCtrlPort())
        elif nodeType == "OM":
            assert(node.isOm == True)
            cmd += node.getOmCmd()
        else:
            assert(0)

        return cmd

    ## Builds the command to gracefully shutdown a node service
    #
    def buildNodeShutdownCmd(self, node, nodeType = None):
        cmd = "./" + node.getShutdownCmd() + " --om_ip=" + self.deployer.getOmIpStr() + \
            " --om_port=" + str(self.deployer.getOmConfPort())
        return cmd

    ## Builds the command to undeploy a node service
    #
    def buildNodeUndeployCmd(self, node, nodeType = None):
        #
        # Forcefully brings down the processes
        #
        cmd = "pkill -9 "
        if nodeType == "SM":
            cmd += node.getSmBin()
        elif nodeType == "DM":
            cmd += node.getDmBin()
        elif nodeType == "OM":
            assert(node.isOm == True)
            cmd += node.getOmBin()
        else:
            assert(0)
        return cmd

    def getNode(self, _id):
        for node in self.nodes:
            if node.nodeId == _id:
                return node
        return None

    def getNodeIdByName(self, _name):
        for node in self.nodes:
            if node.name == _name:
                return node.nodeId

    ## Deploys SM service for a node
    #
    def deploySM(self, _id):
        return self.deployNodeService(_id, "SM")

    ## Undeploys SM service for a node
    #
    def undeploySM(self, _id):
        return self.undeployNodeService(_id, "SM")

    ## Deploys DM service for a node
    #
    def deployDM(self, _id):
        return self.deployNodeService(_id, "DM")

    ## Undeploys DM service for a node
    #
    def undeployDM(self, _id):
        return self.undeployNodeService(_id, "DM")

    ## Deploys OM service for a node
    #
    def deployOM(self, _id):
        return self.deployNodeService(_id, "OM")

    ## Undeploys OM service for a node
    #
    def undeployOM(self, _id):
        return self.undeployNodeService(_id, "OM")

    ## Deploys all services for a node
    #
    def deployNode(self, _id):
        node = self.getNode(_id)
        if node == None:
            return -1

        # When deploying all node services,
        # deploy OM first
        if node.isOm == True:
            result = self.deployOM(_id)
            if result != 0:
                return result

        result = self.deploySM(_id)
        if result != 0:
            return result

        result = self.deployDM(_id)
        if result != 0:
            return result

        return 0

    ## Deploys all service for a node based on name
    #
    def deployNodeByName(self, _name):
        nodeId = self.getNodeIdByName(_name)
        return self.deployNode(nodeId)

    ## Deploys a specific service for a node based on name
    #
    def deployNodeServiceByName(self, _name, _service):
        nodeId = self.getNodeIdByName(_name)
        return self.deployNodeService(nodeId, _service)

    ## Undeploys all services for a node
    #
    def undeployNode(self, _id):
        node = self.getNode(_id)
        if node == None:
            return -1

        result = self.undeployDM(_id)
        if result != 0:
            return result

        result = self.undeploySM(_id)
        if result != 0:
            return result

        # When deploying all node services,
        # deploy OM last
        if node.isOm == True:
            result = self.undeployOM(_id)
            if result != 0:
                return result

        return 0

    ## Undeploys all service for a node based on name
    #
    def undeployNodeByName(self, _name):
        nodeId = self.getNodeIdByName(_name)
        return self.undeployNode(nodeId)

    ## Shuts down a service on a node by name
    #
    def shutdownNodeServiceByName(self, _name, nodeType):
        nodeId = self.getNodeIdByName(_name)
        return self.shutdownNodeService(nodeId, nodeType)
        

    ## Deploys a services for a node
    # from the inventory
    #
    def deployNodeService(self, _id, nodeType):
        # Locate node in inventory
        node = self.getNode(_id)
        if node == None:
            return -1

        cmd = self.buildNodeDeployCmd(node, nodeType)
        started = self.deployer.runNodeCmd(node, cmd, "Starting the server")
        if started == True:
            print "Server %s running on %s..." % (nodeType, node.ipStr)
        else:
            print "Failed to start %s server on %s..." % (nodeType, node.ipStr)
            return -1

        # Sleep to let any async start up task complete...
        time.sleep(5)
        return 0

    ## Undeploys a service for a node
    #
    def undeployNodeService(self, _id, nodeType):
        # Locate node in inventory
        node = self.getNode(_id)
        if node == None:
            return -1

        cmd = self.buildNodeUndeployCmd(node, nodeType)
        stopped = self.deployer.runNodeCmd(node, cmd)
        if stopped == True:
            print "Undeployed %s on %s..." % (nodeType, node.ipStr)
        else:
            print "Failed to undeploy %s server on %s..." % (nodeType, node.ipStr)
            return -1

        return 0

    ## Shuts down a service for a node
    #
    def shutdownNodeService(self, _id, nodeType):
        # Locate node in inventory
        node = self.getNode(_id)
        if node == None:
            return -1

        cmd = self.buildNodeShutdownCmd(node, nodeType)
        shutdown = self.deployer.runNodeCmd(node, cmd)
        if shutdown == True:
            print "Shutdown %s on %s..." % (nodeType, node.ipStr)
        else:
            print "Failed to shutdown %s on %s..." % (nodeType, node.ipStr)
            return -1

        return 0

## Class that manages services and their deployment
#
class ServiceDeploy():
    deployer      = None
    volService    = None
    clientService = None
    nodeService   = None
    verbose       = None
    debug         = None

    def __init__(self, _path, _user, _pswd, _pkey,
                 _omIp, _omConf, _omCtrl, verbose=False, debug=False):
        self.verbose       = verbose
        self.debug         = debug
        self.deployer      = DeployConfig(_path, _user, _pswd, _pkey,
                                     _omIp, _omConf, _omCtrl,
                                     self.verbose, self.debug)
        self.volService    = VolService(self.deployer)
        self.clientService = ClientService(self.deployer)
        self.nodeService   = NodeService(self.deployer)
