#!/usr/bin/python

import os
import os.path
import subprocess
import re
import sys
import time
import traceback
import logging

import pwrCycle
import MySQLdb
import dbaccess

def main():
    #sys.argv.append("00:A0:D1:E3:5A:C0")
    if not len(sys.argv) == 2:
        print "You must specify a MAC address when running this script."
        print "E.g. %s <MAC_ADDRESS>" %sys.argv[0]
        print "E.g. MAC_ADDRESS: 00:A0:D1:E3:57:CB"
        sys.exit(1)
    
    dba = MySQLAccess()
    if not dba.getMachineInfo(sys.argv[1]):
        print "The MAC address %s is not known by the database.  Unable to proceed."
        sys.exit(1)
    
    if BootSetup().setup_config(sys.argv[1]):
        sys.exit(0)
    sys.exit(1)


class MySQLAccess(dbaccess.dbaccess):
    def __init__(self):
        self.dbHost = "qa.maxiscale.com"
        self.dbUser = "tester"
        self.dbPass = "tester"
        self.dbName = "autotest"
        self.db = None
        # Put the log to stdout
        self.log = logging.getLogger("DB_Access")

    def getRows (self, query):
        """ Simple grouping of the connection to db,
        get a query, and close the connection, return
        the row list. """
        self.connect_db()
        rows = self.query(query)
        self.disconnect_db()
        return rows
        
    def getOSid (self, os_name):
        """ The OS is store as an numeric ID in the harness_list, so a second table need to be referenced
        to get the string name of the OS type.  The function return the ID of a given OS name. 
        Using ID is better than string compare because the IDs are always unique, and there
        is no need for string compare. """
        # This is a strict check, not a %almost% check.
        query = "select id from harness_os_list where os_type=\"%s\""%os_name
        rows = self.getRows(query)
        if len (rows) == 0:
            return None
        elif len (rows) > 1:
            # Warn if there is more than 1 id returned.
            print ("Duplicate OS type in the harness_os_list.")
        # If there is value, return the first ID or the only ID
        return rows[0]['id']

    def getMachines (self, status=None, reserve_user=None, currentOS=None, MAC_ADDR=None):
        """ Get the machines in the harness list with optional filters """
        # Form the basic query
        query = "select primary_ip ip, hol.os_type currentOS, SerialCon, "
        query += "SerialCon_port, hname hostname, pdu_name, pdu_port, pdu2_name, pdu2_port from harness_list hl "
        query += "left join harness_os_list hol on hl.currentOS=hol.id"
        
        # Add the where filter
        whereLine = ""
        
        # Check if we filter on status
        if status != None:
            whereLine = " where status=%s"%status
        
        # If there is a reserve_user value, "" = all free machines
        if reserve_user != None:
            if whereLine == "":
                whereLine = " where reserve_user=\"%s\""%reserve_user
            else:
                whereLine += " and reserve_user=\"%s\""%reserve_user
        
        # Filter on currentOS
        if currentOS != None:
            # Current OS is an ID, so we need to get the ID from db first
            id = self.getOSid(currentOS)
            if whereLine == "":
                whereLine = " where currentOS=%s"%id
            else:
                whereLine += " and currentOS=%s"%id
        
        # Filter on MAC_ADDR
        if MAC_ADDR != None:
            if whereLine == "":
                whereLine = " where MAC_ADDR=\"%s\""%MAC_ADDR
            else:
                whereLine += " and MAC_ADDR=\"%s\""%MAC_ADDR
        
        # Combine the lines
        if whereLine != "":
            query += whereLine
        rows = self.getRows(query)
        if len(rows) == 0:
            return None
        else:
            return rows
    
    def getOSDefinition(self, os_type):
        """ Goto the database and return the os_definition from the
        harness_os_list table in autotest. """
        query = "select os_type, os_default \"default\", os_append append, root_dir "
        query += "from harness_os_list where os_type=\"%s\""%os_type
        rows = self.getRows(query)
        if len(rows) < 1:
            return None
        return rows

    def getMachineInfo(self, MAC_ADDR):
        """ Given a MAC_ADDR return the machine's infomation """
        rows = self.getMachines (MAC_ADDR=MAC_ADDR)
        if len (rows) > 1:
            # We got duplicate machines with the same MAC.
            print ("More than one machine was returned with the same MAC address.")
        if len(rows) == 0:
            return None
        else:
            return rows[0]
    
    def updateStatus(self, status, MAC_ADDR):
        """ Update the machine's status field. """
        query = "update harness_list set status=%s where MAC_ADDR=\"%s\""%(status, MAC_ADDR)
        # Do an update
        self.connect_db()
        rows = self.update(query)
        self.disconnect_db()
        if rows <= 0:
            print ("Failed to update status.")
    
    def getFreeMachines(self):
        return getMachines(reserve_user="")


class BootSetup(object):
    """ The purpose of this object is to setup the environment to allow a lab
    host to boot off of the network.  In order to do this, three or four things 
    must be prepared:
    1) An NFS-accessible or iSCSI-accessible root file system must be available 
    to the host to mount during the boot process.
    2) For iSCSI-based hosts, an iSCSI Target must be created.  The Target
    path must then be added to a custom-built initrd which is downloaded by
    the booting client via TFTP when it first starts up.
    3) The DHCP service (dhcpd) must be configured (via its dhcpd.conf
    configuration file) to assign the host a fixed IP address to its
    corresponding MAC address.  The DHCP service must also pass inform the
    client the name of the pxeboot kernel it can retrieve via TFTP.
    4) A pxelinux configuration file must be created which gives the Linux 
    kernel boot-time configuration information.
    
    This script prepares the boot environment based on the values passed into
    the __init__() function below.
    """
    def __init__(self,
                 tftpboot_dir = "/tftpboot/",
                 pxe_dir = "/tftpboot/pxelinux.cfg/", 
                 images_dir = "/export/images/",
                 initrd_dir = "/anna/init_test/",
                 initrd_parent = "/anna/",
                 lockfile = "/anna/lockfile",
                 nfs_host = "192.168.17.3",
                 nfs_export = "/export/images/"):
        """ It is currently assumed that the host running this script will play 
        all of the following roles:
        - It will be the DHCP, PXE, and TFTP server from which all of the 
        boot-time configuration information will come from.
        - It is also currently the NFS server from which the root file systems 
        are mounted.
        - For iSCSI, it is also the server which hosts the iSCSI Logical Units
        and has them accessible via iSCSI Targets.
        
        The default values above are working default values based on the server
        hosting the services at the time the script was written.
        - pxe_dir and images_dir are literal paths on the local file system of
        the host running the script.
        - nfs_host and nfs_export are values relating to the NFS server hosting
        the root file systems.
        """
        self.tftpboot_dir = tftpboot_dir
        self.pxe_dir = pxe_dir
        self.images_dir = images_dir
        self.initrd_dir = initrd_dir
        self.initrd_parent = initrd_parent
        self.lockfile = lockfile
        self.nfs_host = nfs_host
        self.nfs_export = nfs_export

        # This is the db object
        self.db = MySQLAccess()
    
    def setup_config(self, mac_addr, os_type=None):
        """ This is the main method for this class.  It was originally intended
        that this be run as root as write access to /etc/dhcpd.conf is
        required.  However, because the calling process will be the apache web
        server, the root requirement is not enforced.  As such, the mode for
        /etc/dhcpd.conf should allow world-write access.
        
        @type mac_addr: str
        @param mac_addr: This is the MAC address of the host being configured.
        It should be passed in the following form: 00:A0:D1:E3:57:CB
        
        @type os_type: str
        @param os_type: This is the operating system flavor, version, and
        platform type as a string.  The following are a couple of examples 
        of expected input: fedora6_x64 or centos51_x86
        
        If os_type is not passed in, it will be fetched from the database
        based on the MAC address provided.
        
        @return: bool 
        Returns True on success and False if anything along the line failed.
        """
        #if not os.getuid() == 0:
        #    print "This script must run as root."
        #    return False
        self.mac_addr = mac_addr
        
        # First, check if we got an os_type passed in.  If not, try to query it
        # from the database.
        if not os_type:
            try:
                os_type = self.db.getMachineInfo(mac_addr)["currentOS"]
            except:
                print "Failed to fetch the OS type from the database.  The \
                       script cannot continue."
                return False
        
        # Based on the OS type (os_type), we'll query the database for details
        # we'll need to set up that OS type.
        self.os_def = self.db.getOSDefinition(os_type)[0]
        if not self.os_def:
            print "The OS type %s isn't valid.  The script cannot continue." %os_type
            return False

        # Here's an example of what comes back from the database through
        # the self.db.getOSDefinition(os_type) function:
        # ( <-- It comes as a dictionary within a tuple
        #  {"os_type": "ubuntu710_x64",
        #   "default": "vmlinuz-2.6.22-14-ubuntu",
        #   "append": "initrd=initrd.img-2.6.22-14-ubuntu"}
        # )
        
        # ( <-- It comes as a dictionary within a tuple
        #  {"os_type": "fedora6_x64",
        #   "default": "vmlinuz-2.6.22.custom.fc6",
        #   "append": "root=/dev/nfs rw"}
        # )
        
        # ( <-- It comes as a dictionary within a tuple
        #  {"os_type": "centos51_x64",
        #   "default": "vmlinuz-2.6.18-53.1.14.el5",
        #   "append": "rw root=LABEL=iscsiRoot initrd=initrd."}
        # )
        
        if not "bsd" in self.os_def["os_type"]:    
            if not self.__setup_pxelinux():
                print "The setup failed.  The script cannot continue."
                return False

        if not self.__update_dhcpd():
            print "/etc/dhcpd.conf updated failed.  The script cannot continue."
            return False
        
        if "iscsi" in self.os_def["append"]:
            if not self.__setup_ietd():
                print "Setup of ietd.conf failed.  The script cannot continue."
                return False
        
        if not self.__reset_power():
            print "A problem occurred resetting the node's power."
            return False
        
        return True
        
    def __setup_pxelinux(self):
        """ This method creates the pxelinux config file in self.pxe_dir.  It
        only does so if a valid root file system for the desired OS specified
        can be found.  It will also check the desired OS against its known OS
        types.
        
        The pxelinux config file must be in the following format:
        01-00-a0-d1-e3-57-cb
        - The "01-" tells pxelinux that this is Ethernet communication.
        - The MAC address is all in lower case.
        
        The configuration information necessary to build a pxelinux
        configuration file should all come from the database.
        """
        # This is what a pxelinux config file looks like:
        
        # [root@qatest1 pxelinux.cfg]# cat default.fc6
        # DEFAULT vmlinuz-2.6.22.custom.fc6
        # APPEND root=/dev/nfs rw nfsroot=192.168.17.1:/export/images/fedora6_x86-64 ip=dhcp
        
        # [root@qatest1 pxelinux.cfg]# cat default.ub7
        # DEFAULT vmlinuz-2.6.22-14-ubuntu
        # APPEND initrd=initrd.img-2.6.22-14-ubuntu nfsroot=192.168.17.1:/export/images/ubuntu710_x86-64 ip=dhcp
        
        # [root@qatest1 pxelinux.cfg]# cat default.centos51
        # DEFAULT vmlinuz-2.6.18-53.1.14.el5
        # APPEND rw root=LABEL=iscsiRoot initrd=initrd.00a0d1e357c4
        
        if not self.__checkForRootFileSystem():
            return False
        file = open(self.pxe_dir + "01-" + self.mac_addr.lower().replace(":", "-"), "w")
        file.write("DEFAULT %s\n" %self.os_def["default"])
        try:
            if "iscsi" in self.os_def["append"]: # iSCSI-mounted root file system
                file.write("APPEND %s%s\n" %(self.os_def["append"],
                                             self.mac_addr.upper().replace(":", "")))
            else: # NFS-mounted root file system
                file.write("APPEND %s nfsroot=%s:%s%s/%s ip=dhcp\n"
                           %(self.os_def["append"], self.nfs_host, self.nfs_export,
                             self.mac_addr.replace(":", ""), self.os_def["os_type"]))
        except:
            print traceback.format_exc()
        file.close()
        #file = open(self.pxe_dir + mac_addr, "r")
        #print file.read()
        #file.close()
        return True
    
    def __checkForRootFileSystem(self):
        """ A simple helper function to detect if an NFS-mountable root file
        system or an iSCSI Logical Unit with the desired OS for the host can
        be found.  It just checks for the existence of the directory path,
        nothing more.
        
        It looks for something like this on the LOCAL file system:
        /export/images/00A0D1E357CB/fedora6_x64
        -- or --
        /export/images/00A0D1E357CB/freebsd7_x64
        where /export/images/ is self.images_dir, 00A0D1E357CB is mac_addr, and
        fedora6_x64 is the NFS-mountable root directory or contains the iSCSI
        Logical Unit.
        """
        mac_addr = self.mac_addr.replace(":", "")
        if os.path.exists(self.images_dir + mac_addr + "/" + self.os_def["os_type"]):
            return True
        print "A directory containing an NFS-mountable root  file system or " \
              "iSCSI Logical unit was not found.  Expected the following " \
              "directory to exist: %s" \
              %(self.images_dir + mac_addr + "/" + self.os_def["os_type"])
        return False
        
    def __update_dhcpd(self):
        """ This method updates the dhcp.conf file with a suitable record for
        the host which will be booting from the network via PXE and with an
        NFS-mounted root file system.
        """
        host_info = self.db.getMachineInfo(self.mac_addr)
        host_name = host_info["hostname"]
        host_ip = host_info["ip"]
        
        # Set the MAC address characters to upper case for predictability and
        # consistency.
        mac_addr = self.mac_addr.upper()
        
        # The filename line in the host{} section of the dhcpd.conf determines
        # what kernel binary PXE will send down to the host to boot.  For BSD,
        # we do not boot from the network so we don't pass anything.  For Linux,
        # the binary is pxelinux.0
        filename = "pxelinux.0"
        
        # Read into memory the /etc/dhcpd.conf file.  We will look in this for
        # an existing host entry.  This in-memory copy will be updated with
        # the new host record data and be written out.
        try:
            file = open("/etc/dhcpd.conf", "r")
            conf = file.read()
            file.close()
            conf = conf.replace("\r\n", "\n")
        except:
            print "Failed to open /etc/dhcpd.conf for read.  Exiting."
            return False
        
        # This is an example of the host{} section of dhcpd.conf.  Each host
        # present in dhcpd.conf will have an entry that defines the MAC 
        # address, the fixed IP, the kernel binary to load (filename) and the
        # host name.
        #host r02s02 {
        #  hardware ethernet 08:00:2b:4c:59:23;
        #  fixed-address 239.252.197.9;
        #  filename "pxelinux.0";
        #  option host-name "r02s02";
        #}
        #hostr02s01 {
        #  hardware ethernet 08:00:2b:4c:59:23;
        #  fixed-address 239.252.197.9;
        #  option host-name "r02s01";
        #}
        # Use these patterns to find Linux and BSD hosts in within the
        # dhcpd.conf file.
        host_finder_linux = r'host \w* {\s*hardware ethernet (?P<MAC_ADDR>\w\w:\w\w:\w\w:\w\w:\w\w:\w\w);\s*fixed-address (?P<IP_ADDR>\d*\.\d*\.\d*\.\d*);\s*filename "(?P<FILE_NAME>.*)";\s*option host-name "(?P<HOST_NAME>\w*)";\s*}\s*'
        host_finder_bsd = r'host \w* {\s*hardware ethernet (?P<MAC_ADDR>\w\w:\w\w:\w\w:\w\w:\w\w:\w\w);\s*fixed-address (?P<IP_ADDR>\d*\.\d*\.\d*\.\d*);\s*option host-name "(?P<HOST_NAME>\w*)";\s*}\s*'
        find_this_linux = re.compile(host_finder_linux, re.MULTILINE)
        find_this_bsd = re.compile(host_finder_bsd, re.MULTILINE)
        found_hosts = re.findall(find_this_linux, conf)
        found_hosts.extend(re.findall(find_this_bsd, conf))
        
        # Iterate through the found host entries and see if our client's MAC
        # address is found in there.  If more than 1, get out.  The admin
        # needs to remove one.  If 1 is found, remove it.  Finally, add a new
        # record based on the OS type.
        counter = 0
        for host in found_hosts:
            if mac_addr in host:
                counter += 1
        if counter > 1:
            print "MAC address %s appears in /etc/dhcpd.conf more than once." %mac_addr
            print "Remove the duplicate entries and rerun the script."
            return False
        if counter == 1: # Remove the found host - we'll add it back at the end.
            host_finder_linux = host_finder_linux.replace("\w\w:\w\w:\w\w:\w\w:\w\w:\w\w", mac_addr)
            host_finder_bsd = host_finder_bsd.replace("\w\w:\w\w:\w\w:\w\w:\w\w:\w\w", mac_addr)
            find_this_linux = re.compile(host_finder_linux, re.MULTILINE)
            find_this_bsd = re.compile(host_finder_bsd, re.MULTILINE)
            try:
                host_record = re.search(find_this_linux, conf).group(0)
                conf = conf.replace(host_record, "") # Remove the record
            except AttributeError:
                pass # If it didn't find it it's a BSD record.
            try:
                host_record = re.search(find_this_bsd, conf).group(0)
                conf = conf.replace(host_record, "") # Remove the record
            except AttributeError:
                pass # If it didn't find it it's a Linux record.
        
        # Now add a new host record to dhcpd.conf replacing the one that was
        # removed (above) or adding one that never existed if counter == 0.
        # First, generate the host record entry as a string.
        new_host = "host %s {\n" %host_name
        new_host += "  hardware ethernet %s;\n" %mac_addr
        new_host += "  fixed-address %s;\n" %host_ip
        if not "bsd" in self.os_def["os_type"]:
            new_host += "  filename \"%s\";\n" %filename
        new_host += "  option host-name \"%s\";\n" %host_name
        new_host += "}\n\n"
            
        # Finally, add the host record just before the 
        # "next-server 192.168.17.1;" line.  In effect, this should append 
        # the host record entry to the bottom of the list os hosts in the
        # dhcpd.conf file.
        get_last_line = re.compile(r'next-server \d*\.\d*\.\d*\.\d*')
        last_line = re.search(get_last_line, conf).group(0)
        conf = conf.replace(last_line, new_host + last_line)
        if not self.__restartDhcpd(conf):
            return False
        return True
    
    def __restartDhcpd(self, contents):
        """ Simple method to write out the updated dhcpd.conf file and restart
        the dhcpd service.  In an effort to maximize data protection, I will
        not write into the "real" dhcpd.conf file until I already have a copy
        of the new file written out to disk.  In a catastropic situation such
        as losing power during the middle of the write, we'll have the
        dhcpd.conf.update to go back to.
        """
        try:
            file = open("/etc/dhcpd.conf.update", "w")
            file.write(contents)
            file.close()
            file = open("/etc/dhcpd.conf", "w")
            file.write(contents)
            file.close()
            pipe = subprocess.Popen("service dhcpd restart", shell=True,
                                    stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
            output = pipe.stdout.read()
            print "Output of dhcpd restart: %s" %output
            return True
        except:
            return False

    def __setup_ietd(self):
        mac_addr = self.mac_addr.upper().replace(":", "")
        file = open("/etc/ietd.conf", "r")
        ietd_contents = file.read()
        file.close()
        # We're looking for iSCSI targets in the /etc/ietd.conf file.  The
        # targets should be defined as follows:
        # Target iqn.2008-04.com.maxiscale.lab:storage.00A0D1E35B7E
        # Lun 0 Path=/export/images/00A0D1E35B7E/centos51_x64/iscsiRoot,Type=fileio
        # The following regex should find these targets:
        target_finder = r"^Target iqn.2008-04.com.maxiscale.lab:storage.(\w*)\s*Lun 0 Path=%s(\w*)/%s/iscsiRoot,Type=fileio\n\n" %(self.images_dir, self.os_def["os_type"])
        find_this_target = re.compile(target_finder, re.MULTILINE)
        found_targets = re.findall(find_this_target, ietd_contents)
        
        # Iterate through the found target entries and see if our client's MAC
        # address is found in there.  If more than 1 is found, get out.  The 
        # admin needs to remove one.  If 1 is found, remove it.  We'll add a
        # new target for this MAC address at the end.
        counter = 0
        for target in found_targets:
            if mac_addr in target:
                counter += 1
        if counter > 1:
            print "MAC address %s appears in /etc/ietd.conf %s times." %(mac_addr, counter)
            print "It should appear no more than once."
            print "Remove the duplicate entries and rerun the script."
            return False
        if counter == 1:
            # Remove the found target - we'll add it back at the end.
            target_finder = r"^Target iqn.2008-04.com.maxiscale.lab:storage.(%s)\s*Lun 0 Path=%s(%s*)/%s/iscsiRoot,Type=fileio\n\n" %(mac_addr, self.images_dir, mac_addr, self.os_def["os_type"])
            find_this_target = re.compile(target_finder, re.MULTILINE)
            tgt = re.search(find_this_target, ietd_contents).group(0)
            ietd_contents = ietd_contents.replace(tgt, "")
        # Add the new target at the end
        ietd_contents += "Target iqn.2008-04.com.maxiscale.lab:storage.%s\n" %mac_addr
        ietd_contents += "Lun 0 Path=%s%s/%s/iscsiRoot,Type=fileio\n\n" %(self.images_dir, mac_addr, self.os_def["os_type"])
        
        if not self.__create_initrd():
            return False
        
        try:
            #file = open("/etc/ietd.conf.update", "w")
            #file.write(ietd_contents)
            #file.close()
            #file = open("/etc/ietd.conf", "w")
            #file.write(ietd_contents)
            #file.close()
            #pipe = subprocess.Popen("service iscsi-target restart", shell=True,
            #                        stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
            #output = pipe.stdout.read()
            #print "Output of iscsi-target restart: %s" %output
            return True
        except:
            return False
        
    def __create_initrd(self):
        locked = False
        for i in range(1000):
            try:
                fd = os.open(self.lockfile, os.O_CREAT|os.O_WRONLY|os.O_EXCL)
                os.close(fd)
                locked = True
                break
            except:
                time.sleep(0.01)
        if not locked:
            print "Unable to establish a lock on the initrd directory."
            return False
        mac_addr = self.mac_addr.upper().replace(":", "")
        file = open(self.initrd_dir + "init", "r")
        init_contents = ""
        for line in file:
            if "iscsistart" in line:
                line = re.sub(r"\w{12}", mac_addr, line, 1)
            init_contents += line
        file.close()
        try:
            file = open(self.initrd_dir + "init.update", "w")
            file.write(init_contents)
            file.close()
            file = open(self.initrd_dir + "init", "w")
            file.write(init_contents)
            file.close()
            os.remove(self.initrd_dir + "init.update")
        except:
            return False
        orig_cwd = os.getcwd()
        os.chdir(self.initrd_dir)
        pipe = subprocess.Popen("find . | cpio --quiet -c -o | gzip -9 -n > %sinitrd.%s"
                                %(self.tftpboot_dir, mac_addr), shell=True,
                                  stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        output = pipe.stdout.read()
        print "Output of cpio command to create initrd file: %s" %output
        os.chdir(orig_cwd)
        os.remove(self.lockfile)
        return True


    def __reboot_Machine (self):
        """
        Assume SSH keys have be setup.
        """
        host_info = self.db.getMachineInfo(self.mac_addr)
        command = "ssh root@%s shutdown -r now"%host_info['ip']
        pipe = subprocess.Popen (command, shell=True,
                                stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        output = pipe.stdout.read ()
        print "Reboot %s: %s"%(host_info['ip'], output)
        return True

    
    def __reset_power(self):
        """
        """
        host_info = self.db.getMachineInfo(self.mac_addr)
        pwrCycle.Pcycler(host_info["pdu_port"], host_info["pdu_name"], "on")
        if host_info["pdu2_name"]:
            pwrCycle.Pcycler(host_info["pdu2_port"], host_info["pdu2_name"], "on")
        return True


if __name__ == "__main__":
    logging.basicConfig(level=logging.INFO,
                       format='%(asctime)s %(name)-24s %(levelname)-8s %(message)s',
                       datefmt='%Y-%m-%d %H:%M:%S')
    main()
