#
# Copyright 2013 Formation Data Systems, Inc.
#
import optparse
import paramiko
import ConfigParser
import re
import pdb
import traceback

verbose = False
debug = False

#
# Defines a volume in the cluster
#
class StorVol():
    name   = None
    volId  = None
    size   = None
    policy = None
    client = None
    access = None
    cliBin = "fdscli"

    def __init__(self, _name):
        self.name = _name

    def setId(self, _id):
        self.volId = _id

    def setSize(self, _size):
        self.size = _size

    def setPolicy(self, _polId):
        self.policy = _polId

    def setClient(self, _clientName):
        self.client = _clientName

    def setAccess(self, _accessType):
        self.access = _accessType

    def getCrtCmd(self):
        return "%s --volume-create %s -i %d -s %d -p %d -y %s" % (self.cliBin,
                                                                  self.name,
                                                                  self.volId,
                                                                  self.size,
                                                                  self.policy,
                                                                  self.access)

    def getAttCmd(self):
        # The client name is hard coded for now because
        # it is hard coded in the SH. As a result, the
        # local client variable is ignored.
        return "%s --volume-attach %s -i %d -n localhost-%s" % (self.cliBin,
                                                                self.name,
                                                                self.volId,
                                                                self.client)


#
# Defines a policy in the cluster
#
class StorPol():
    name     = None
    polId    = None
    iopsmin  = None
    iopsmax  = None
    priority = None
    volType  = "disk"
    cliBin = "fdscli"

    def __init__(self, _name):
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
        return "%s --policy-create %s -p %d -g %d -m %d -r %d" % (self.cliBin,
                                                                  self.name,
                                                                  self.polId,
                                                                  self.iopsmin,
                                                                  self.iopsmax,
                                                                  self.priority)

    def getDelCmd(self):
        return "%s --policy-delete %s -p %d" % (self.cliBin,
                                                self.name,
                                                self.polId)

    def setVolType(self, _type):
        if _type == "ssd" or _type == "disk" or _type == "hybrid" or _type == "hybrid_prefcap":
            self.volType = _type
        else:
            print "Unknown volume type:", _type, ", set default to disk"
            self.volType = "disk"

    def getTierCmd(self, opt, vol_id):
        return "%s --auto-tier=%s --vol-type=%s --volume-id=%d" % (
            self.cliBin, opt, self.volType, vol_id
        )

#
# Defines a client in the cluster
#
class StorClient():
    ipStr = None
    name  = None
    isBlk = False
    logSeverity = 2
    amBin  = "AMAgent"
    ubdBin = "ubd"
    blkDir = "stor_hvisor"
    blkMod = "fbd.ko"

    def __init__(self, _name):
        self.name = _name

    def setIp(self, _ip):
        self.ipStr = _ip

    def setBlk(self, _blk):
        self.isBlk = _blk

    def setLogSeverity(self, _sev):
        self.logSeverity = _sev

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
#
# Defines a node in the cluster
#
class StorNode():
    ipStr = "127.0.0.1"
    name = None
    controlPort = 6900
    dataPort = 7900
    configPort = None
    isOm = False
    logSeverity = 2
    omBin = "orchMgr"
    dmBin = "DataMgr"
    smBin = "StorMgr"
    
    def __init__(self, _name):
        self.name = _name

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
    
    def setLogSeverity(self, _sev):
        self.logSeverity = _sev
        
    def getOmCmd(self):
        return "%s --port=%d --prefix=%s_" % (self.omBin,
                                              self.configPort,
                                              self.name)

    def getSmCmd(self):
        return "%s --port=%d --cp_port=%d --prefix=%s_ --log-severity=%d" % (self.smBin,
                                                          self.dataPort,
                                                          self.controlPort,
                                                          self.name,
                                                          self.logSeverity)

    def getDmCmd(self):
        return "%s --port=%d --cp_port=%d --prefix=%s_ --log-severity=%d" % (self.dmBin,
                                                           self.dataPort + 1,
                                                           self.controlPort + 1,
                                                           self.name,
                                                           self.logSeverity)

    def getOmBin(self):
        return self.omBin

    def getSmBin(self):
        return self.smBin

    def getDmBin(self):
        return self.dmBin

    def getLogSeverity(self):
        return self.logSeverity 
#
# Defines how to bring up a cluster
#
class TestBringUp():
    #
    # Config and cluster elements
    #
    cfgFile = None
    nodes   = []
    clients = []
    volumes = []
    policies = []

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

    #
    # SSH login params
    #
    sshUser = None
    sshPswd = None
    sshPort = 22

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
    ldLibPath    = "export LD_LIBRARY_PATH=$LD_LIBRARY_PATH"
    iceHome      = "export ICE_HOME="

    def __init__(self, _cfg_file):
        self.cfgFile = _cfg_file
        self.nodes = []

    #
    # Sets up cluster path variables
    #
    def setPath(self, _path):
        self.srcPath = _path
        self.fdsIceDir = self.srcPath + "/" + self.fdsIceDir
        self.fdsLdbLibDir = self.srcPath + "/" + self.fdsLdbLibDir
        self.fdsIceLibDir = self.srcPath + "/" + self.fdsIceLibDir

        self.fdsBinDir = self.srcPath + "/" + self.fdsBinDir
        self.ldLibPath = self.ldLibPath + ":" + self.fdsLdbLibDir + ":" + self.fdsIceLibDir + ":" + self.fdsBstLibDir
        self.iceHome = self.iceHome + self.srcPath + "/" + self.fdsIceDir


    #
    # Loads a volume from items
    # in the cfg file
    #
    def loadPolicy(self, name, items):
        policy = StorPol(name)
        for item in items:
            key   = item[0]
            value = item[1]
            if key == "id":
                policy.setId(int(value))
            elif key == "iops_min":
                policy.setIopsMin(int(value))
            elif key == "iops_max":
                policy.setIopsMax(int(value))
            elif key == "priority":
                policy.setPriority(int(value))
            elif key == "vol_type":
                policy.setVolType(value)
            else:
                print "Unknown item %s, %s in %s" % (key, value, name)

        self.policies.append(policy)

    # Get the policy config obj matching the policy id.
    #
    def getPolicy(self, pol_id):
        for pol in self.policies:
            if pol.polId == pol_id:
                return pol

        print "Can't find policy id ", pol_id
        return None

    #
    # Loads a volume from items
    # in the cfg file
    #
    def loadVol(self, name, items):
        volume = StorVol(name)
        for item in items:
            key   = item[0]
            value = item[1]
            if key == "id":
                volume.setId(int(value))
            elif key == "size":
                volume.setSize(int(value))
            elif key == "policy":
                volume.setPolicy(int(value))
            elif key == "client":
                volume.setClient(value)
            elif key == "access":
                volume.setAccess(value)
            else:
                print "Unknown item %s, %s in %s" % (key, value, name)

        self.volumes.append(volume)

    #
    # Loads a client from items
    # in the cfg file
    #
    def loadClient(self, name, items):
        client = StorClient(name)
        for item in items:
            key   = item[0]
            value = item[1]
            if key == "ip":
                client.setIp(value)
            elif key == "blk":
                if value == "true":
                    client.setBlk(True)
                else:
                    client.setBlk(False)
            elif key == "log_severity":
                client.setLogSeverity(int(value))
            else:
                print "Unknown item %s, %s in %s" % (key, value, name)

        self.clients.append(client)

    #
    # Loads a node from items
    # in the cfg file
    #
    def loadNode(self, name, items):
        node = StorNode(name)
        for item in items:
            key = item[0]
            value = item[1]
            if key == "om":
                if value == "true":
                    node.setOm(True)
                else:
                    node.setOm(False)
            elif key == "ip":
                node.setIp(value)
            elif key == "control_port":
                node.setCp(int(value))
            elif key == "data_port":
                node.setDp(int(value))
            elif key == "config_port":
                node.setConfPort(int(value))
            elif key == "log_severity":
                node.setLogSeverity(int(value))
            else:
                print "Unknown item %s, %s in %s" % (key, value, name)
        
        if node.isOm == True:
            self.omIpStr    = node.ipStr
            self.omConfPort = node.configPort
            if (self.omIpStr == None) or (self.omConfPort == None):
                print "Error: An OM port and IP must be specified"
                return -1

        self.nodes.append(node)

    #
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

    #
    # Builds the command to bring up a node service
    #
    def buildNodeCmd(self, node, nodeType = None):
        cmd = self.ldLibPath + "; " + self.iceHome + "; " + "cd " + self.fdsBinDir + "; " + "ulimit -c unlimited;" + "./"
        if nodeType == "SM":
            cmd += node.getSmCmd()
            cmd += " --om_ip=%s --om_port=%d" % (self.omIpStr,
                                                 self.omConfPort)
        elif nodeType == "DM":
            cmd += node.getDmCmd()
            cmd += " --om_ip=%s --om_port=%d" % (self.omIpStr,
                                                 self.omConfPort)
        elif node.isOm == True:
            cmd += node.getOmCmd()
        else:
            assert(0)

        return cmd

    #
    # Builds the command to insert a kernel module
    #
    def buildModCmd(self, client):
        cmd = client.getBlkCmd(self.srcPath)
        return cmd

    #
    # Builds the command to start SH UBD service
    #
    def buildUbdCmd(self, client):
        cmd = self.ldLibPath + "; " + self.iceHome + "; " + " cd " + self.fdsBinDir + "; " + "ulimit -s 4096; " +  "ulimit -c unlimited;" + "./" + client.getUbdCmd() + " --om_ip=" + self.omIpStr + " --om_port=" + str(self.omConfPort) + " --node_name=localhost-" + client.name + " --log-severity=" + str(client.getLogSeverity())
        return cmd

    #
    # Builds the command to start SH AM service
    #
    def buildAmCmd(self, client):
        cmd = self.ldLibPath + "; " + self.iceHome + "; " + " cd " + self.fdsBinDir + "; " + "ulimit -s 4096; " + "ulimit -c unlimited; " + "./" + client.getAmCmd() + " --om_ip=" + self.omIpStr + " --om_port=" + str(self.omConfPort) + " --node_name=localhost-" + client.name + " --log-severity=" + str(client.getLogSeverity())
        return cmd

    #
    # Builds the command to create a policy
    #
    def buildPolCrtCmd(self, policy):
        cmd = self.ldLibPath + "; " + self.iceHome + "; " + " cd " + self.fdsBinDir + "; " + "./" + policy.getCrtCmd() + " --om_ip=" + self.omIpStr + " --om_port=" + str(self.omConfPort)
        return cmd

    #
    # Builds the command to delete a policy
    #
    def buildPolDelCmd(self, policy):
        cmd = self.ldLibPath + "; " + self.iceHome + "; " + " cd " + self.fdsBinDir + "; " + "./" + policy.getDelCmd() + " --om_ip=" + self.omIpStr + " --om_port=" + str(self.omConfPort)
        return cmd

    #
    # Builds the command to create a volume
    #
    def buildVolCrtCmd(self, volume):
        cmd = self.ldLibPath + "; " + self.iceHome + "; " + " cd " + self.fdsBinDir + "; " + "./" + volume.getCrtCmd() + " --om_ip=" + self.omIpStr + " --om_port=" + str(self.omConfPort)
        return cmd

    #
    # Builds the command to attach a volume
    #
    def buildVolAttCmd(self, volume):
        cmd = self.ldLibPath + "; " + self.iceHome + "; " + " cd " + self.fdsBinDir + "; " + "./" + volume.getAttCmd() + " --om_ip=" + self.omIpStr + " --om_port=" + str(self.omConfPort)
        return cmd

    #
    # Builds the command to setup volume tier type.
    #
    def buildVolTierCmd(self, volume):
        policy = self.getPolicy(volume.policy)
        if policy is None:
            return None
        cmd = (self.ldLibPath + "; " + self.iceHome + "; " + "cd " +
               self.fdsBinDir + "; ./" + policy.getTierCmd("on", volume.volId) +
               " --om_ip=" + self.omIpStr + " --om_port=" +
               str(self.omConfPort))
        return cmd

    #
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
            client.connect(ipOver, self.sshPort, self.sshUser, self.sshPswd)

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
                while line != "":
                    if verbose == True:
                        print line
                    if re.search(checkStr, line):
                        outputChecked = True
                        break
                    line = stderr.readline()

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


    #
    # Create policy in the cluster
    #
    def polBringUp(self, policy):
        #
        # Runs this command on the OM IP
        #
        crtCmd = self.buildPolCrtCmd(policy)
        result = self.runNodeCmd(policy, crtCmd, ipOver=self.omIpStr)
        if result == True:
            print "Created policy with OM on %s..." % (self.omIpStr)
        else:
            print "Failed to create policy"
            return -1

        return 0


    #
    # Delete policy from the cluster
    #
    def polBringDown(self, policy):
        #
        # Runs this command on the OM IP
        #
        delCmd = self.buildPolDelCmd(policy)
        result = self.runNodeCmd(policy, delCmd, ipOver=self.omIpStr)
        if result == True:
            print "Deleted policy with OM on %s..." % (self.omIpStr)
        else:
            print "Failed to delete policy"
            return -1

        return 0


    #
    # Create and attach volume to cluster
    #
    def volBringUp(self, volume):
        #
        # Runs this command on the OM IP
        #
        crtCmd = self.buildVolCrtCmd(volume)
        result = self.runNodeCmd(volume, crtCmd, ipOver=self.omIpStr)
        if result == True:
            print "Created volume with OM on %s..." % (self.omIpStr)
        else:
            print "Failed to create volume"
            return -1

        #
        # Runs this command on the OM IP
        #
        attCmd = self.buildVolAttCmd(volume)
        result = self.runNodeCmd(volume, attCmd, ipOver=self.omIpStr)
        if result == True:
            print "Attached volume with OM on %s..." % (self.omIpStr)
        else:
            print "Failed to attach volume"
            return -1

        #
        # Run this tier policy command on the OM IP
        #
        tierCmd = self.buildVolTierCmd(volume)
        if tierCmd is None:
            return 0

        result = self.runNodeCmd(None, tierCmd, ipOver=self.omIpStr)
        if result == True:
            print "Setup volume tier command with OM on %s..." % self.omIpStr
        else:
            print "Failed to setup volume tier command"
            return -1
        return 0

    #
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

    #
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
                started = self.runNodeCmd(node, cmd, "accepting tcp connections")
                if started == True:
                    print "Server OM running on %s..." % (node.ipStr)
                else:
                    print "Failed to start OM server..."
                    return -1

            #
            # Bring up SM
            #
            cmd = self.buildNodeCmd(node, "SM")
            started = self.runNodeCmd(node, cmd, "accepting tcp connections")            
            if started == True:
                print "Server SM running on %s..." % (node.ipStr)
            else:
                print "Failed to start SM server..."
                return -1

            #
            # Bring up DM
            #
            cmd = cmd = self.buildNodeCmd(node, "DM")
            started = self.runNodeCmd(node, cmd, "accepting tcp connections")            
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

        return 0

    #
    # Bring up client/SH services. This may
    # bring up either block-based or object-based
    # services
    #
    def clientBringUp(self, client):
        try:
            cmd = None
            #
            # Run remote cmd based on client type
            #
            if client.isBlk == True:
                #
                # Start blktap kernel module
                #
                cmd = self.buildModCmd(client)
                started = self.runNodeCmd(client, cmd)
                if started == True:
                    print "Client blk module running on %s..." % (client.ipStr)
                else:
                    print "Failed to start blk module..."
                    return -1

                #
                # Start UBD user space process
                #
                cmd = self.buildUbdCmd(client)
                started = self.runNodeCmd(client, cmd, "tcp connection established")
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
                started = self.runNodeCmd(client, cmd, "tcp connection established")
                if started == True:
                    print "Client AM running on %s..." % (client.ipStr)
                else:
                    print "Failed to start AM..."
                    return -1

        except Exception, e:
            print "*** Caught exception %s: %s" % (e.__class__, e)
            traceback.print_exc()
            try:
                client.close()
            except:
                pass
            return -1

        return 0

    #
    # Bring down client/SH services. This may
    # bring down either block-based or object-based
    # services
    #
    def clientBringDown(self, client):
        try:
            if client.isBlk == True:
                #
                # Bring down UBD user space process
                #
                ubdCmd = "pkill -9 " + client.getUbdCmd()
                result = self.runNodeCmd(client, ubdCmd)
                if result == True:
                    print "Brought down UBD on %s..." % (client.ipStr)
                else:
                    print "Failed to bring down UBD"
                    return -1

                #
                # Remove blktap kernel module
                #
                rmCmd  = client.getRmCmd(self.srcPath)
                result = self.runNodeCmd(client, rmCmd)
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
                result = self.runNodeCmd(client, amCmd)
                if result == True:
                    print "Brought down AM on %s..." % (client.ipStr)
                else:
                    print "Failed to bring down AM"
                    return -1
        
        except Exception, e:
            print "*** Caught exception %s: %s" % (e.__class__, e)
            traceback.print_exc()
            try:
                client.close()
            except:
                pass
            return -1

        return 0


    #
    # Bring up policies across a cluster
    #
    def bringUpPols(self):
        for policy in self.policies:
           result = self.polBringUp(policy)
           if result != 0:
               return result
        
        return 0

    #
    # Bring down policies across a cluster
    #
    def bringDownPols(self):
        for policy in self.policies:
           result = self.polBringDown(policy)
           if result != 0:
               return result

        return 0

    #
    # Bring up volumes across a cluster
    #
    def bringUpVols(self):
        for volume in self.volumes:
           result = self.volBringUp(volume)
           if result != 0:
               return result
        
        return 0

    #
    # Bring up services on all cluster nodes
    #
    def bringUpNodes(self):
        #
        # Bring up a node with OM service first
        #
        for node in self.nodes:
            if node.isOm == True:
                result = self.nodeBringUp(node)
                if result != 0:
                    return result
                upAlready = node
                break

        #
        # Bring up other node services
        #
        for node in self.nodes:
            if node != upAlready:
                result = self.nodeBringUp(node)
                if result != 0:
                    return result
        return 0

    #
    # Bring down services on all cluster nodes
    #
    def bringDownNodes(self):
        for node in self.nodes:
            result = self.nodeBringDown(node)
            if result != 0:
                return result
        return 0

    #
    # Bring up services on all cluster clients
    #
    def bringUpClients(self):
        for client in self.clients:
            result = self.clientBringUp(client)
            if result != 0:
                return result
        return 0

    #
    # Bring down services on all cluster clients
    #
    def bringDownClients(self):
        for client in self.clients:
            result = self.clientBringDown(client)
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
        result = bu.bringDownPols()
        if result != 0:
            print "Failed to delete all policies"
        else:
            print "Deleted all policies"
        result = bu.bringDownNodes()
        if result != 0:
            print "Failed to bring down all nodes"
        else:
            print "Brought down all nodes"
