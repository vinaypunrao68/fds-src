#!/usr/bin/env python

import optparse
import os
import re
import subprocess
import sys
import xml.etree.ElementTree as element_tree
#import pdb

# Turn on debugging
debug_on = False
header_output = False

# Disk information parsed from system and vendor tools
class Disk:
    # Static disk parsing stuff
    #
    DSK_PREFIX = '/dev/'

    DSK_TYP_SSD = 'SSD'
    DSK_TYP_HDD = 'HDD'
    DSK_TYP_UNKNOWN = 'UKN'

    DISK_BUS_SATA = 'SATA'
    DISK_BUS_SAS = 'SAS'
    DISK_BUS_NA = 'NA'

    p_dev = re.compile('sd[a-z]{1,2}$')

    # We are using 1000*1000
    # device size with M = 1024*1024:     1907729 MBytes
    # device size with M = 1000*1000:     2000398 MBytes (2000 GB)
    DSK_GSIZE = 1000 * 1000 * 1000

    # Save lshw result once
    dsk_lshw_xml = None

## Public member functions
## -----------------------
    # Parse the disk information
    def __init__(self, path, virtualized, is_os_disk):
        assert path != None
        assert re.match(Disk.DSK_PREFIX, path)

        self.dsk_path = path
        self.dsk_cap  = -1
        self.dsk_typ  = Disk.DSK_TYP_UNKNOWN
        self.dsk_formatted = 'Unknown'
        self.dsk_index_use = False
        self.dsk_bus = Disk.DISK_BUS_NA
        self.dsk_os_use = is_os_disk

        self.target = None

        # parse disk information
        self.__parse_with_lshw(path, virtualized)

    # drive type: HDD|SSD
    def get_type(self):
        return self.dsk_typ

    # get os_use
    def get_os_use(self):
        return self.dsk_os_use

    # file system path of disk
    def get_path(self):
        return self.dsk_path

    # set_index, mark this device as an index storage device
    def set_index(self):
        self.dsk_index_use = True

    # set_bus, configure this device's bus type, only used by HDDs
    def set_bus(self, bus_type):
        self.dsk_bus = bus_type

    # print all the disk details
    def print_disk(self, disk_file = None):
        global header_output
        dest = sys.stdout

        if disk_file is not None:
            dest = disk_file

        if not header_output:
            print >>dest, '#path      os_use  index_use  type  bus   capacity (GB)'
            header_output = True
        if self.dsk_cap > 0:
            print >>dest, '%-11s%-8s%-11s%-6s%-6s%-d' % (self.dsk_path, self.dsk_os_use, self.dsk_index_use, self.dsk_typ, self.dsk_bus, self.dsk_cap)

## ----------------------------------------------------------------
    # get the device /sys/block path (e.g. /sys/block/sdb for /dev/sdb)
    @staticmethod
    def get_sysblock_path (path):
        dev = re.split('/', path)
        assert len(dev) == 3
        return '/sys/block/' + dev[2]
 
    # get all Disk objects in the system, including the boot device(s)
    @staticmethod
    def sys_disks (virtualized, os_device_list):
        dev_list  = []
        path_list = Disk.__get_sys_disks_path()

        for path in path_list:
            disk = Disk (path, virtualized, path in os_device_list)
            dev_list.append(disk)

        return dev_list

    ###
    # parse using lshw and xml
    def __parse_with_lshw(self, path, virtualized):
        # Parse for Disk type, capacity
        if Disk.dsk_lshw_xml == None:
            dev_stdout = subprocess.Popen(['lshw', '-xml', '-class', 'disk', '-quiet'], stdout=subprocess.PIPE).stdout
            Disk.dsk_lshw_xml = element_tree.parse(dev_stdout)

        tree = Disk.dsk_lshw_xml
        root = tree.getroot()
        for node in root.findall('node'):
            type=node.get('id')
            if type == 'cdrom':
                continue
            target=type.partition(':')
            self.target = target[2]
            node_logicalname = node.find('logicalname')

            if node_logicalname is None:
                continue

            if node_logicalname.text != path:
                continue

            node_size = node.find('size')

            if node_size is None:
                continue

            units = node_size.get('units')

            if units == 'bytes':
                self.dsk_cap = int(node_size.text) / Disk.DSK_GSIZE
                break
            else:
                self.dsk_cap = 0
                print 'WARNING: lshw units size not implemented for:  ', node_logicalname.text, ', ignoring. '

        if self.dsk_cap < 0:
            print 'WARNING: Size not detected for " ', path, '", will ignore this device'

        # match type
        sysblock_path = Disk.get_sysblock_path(path)
        fp = open(sysblock_path + '/queue/rotational', 'r')
        rotation = int (fp.read().strip())
        fp.close()

        if virtualized:                        # Virtualized, treat as HDD's, can tune in the config file if desired
            self.dsk_typ = Disk.DSK_TYP_HDD
        elif rotation == 0:                  # SSD
            self.dsk_typ = Disk.DSK_TYP_SSD
        elif rotation > 0:                   # HDD
            self.dsk_typ = Disk.DSK_TYP_HDD
                                               # else this device is on a hardware controller,
                                               # so leave the dsk_typ set to the default UKN (unknown)

    @staticmethod
    def __get_sys_disks_path():
        path_info = subprocess.Popen(['ls', Disk.DSK_PREFIX], stdout=subprocess.PIPE).stdout
        path_list = []
        for rec in path_info:
            dsk = Disk.p_dev.match(rec)
            if dsk:
                dbg_print('Found Disk: ' + Disk.DSK_PREFIX + dsk.group(0))
                path_list.append(Disk.DSK_PREFIX + dsk.group(0))
        return path_list

## -------------------------------------------------------------------------------

def dbg_print(msg):
    if debug_on:
        if len (msg) > 0:
            print 'Debug: ' + msg
        else:
            print msg

def debug_dump (devlist):
    print "==========="
    print "DEBUG DUMP"
    print "==========="
    for disk in devlist:
        disk.print_disk()

def get_device_list(fs) :
    device_list = []

    real_fs = os.path.realpath(fs)

    if "/dev/md" in real_fs:
        call_list = ['mdadm', '--detail', real_fs]
        output = subprocess.Popen (call_list, stdout=subprocess.PIPE).stdout
        for line in output:
            if "/dev/sd" in line:
                items = line.strip ('\r\n').split()
                device=items[6].rstrip('0123456789')
                if device not in device_list:
                    device_list.append (device)
    elif "/dev/dm" in real_fs: #lvm device
        sysblock_path = Disk.get_sysblock_path(real_fs)
        call_list = ['ls', sysblock_path + '/slaves']
        output = subprocess.Popen (call_list, stdout=subprocess.PIPE).stdout
        for line in output:
            if "sd" in line:
                items = line.strip ('\r\n').split()
                device=items[0].rstrip('0123456789')
                device = '/dev/' + device
                if device not in device_list:
                    device_list.append (device) 
    else:
        device = real_fs.rstrip('0123456789')
        if device not in device_list:
            device_list.append (device)
    return device_list

def discover_os_devices ():
    '''
    Auto discover the OS disks

    returns:
        A list of devices that are used by the OS
    '''

    os_device_list = []
    root_device = ""

    output = subprocess.Popen(["df", "/"], stdout=subprocess.PIPE).stdout

    for line in output:
        items = line.strip ('\r\n').split()
        if "/" == items[5]:
            root_device = items[0]

    if 0 == len(root_device):
        print "Unable to find the root file system in 'df' output.  Can not continue."
        sys.exit(1)

    os_device_list = get_device_list(root_device)

    return os_device_list

def disk_type_with_controller_tool (controller_tool):
    '''
    inputs:
        controller_tool:  Full path and binary name of the controller tool (storcli64/perccli64 or arcconf or sas2ircu)

    returns:
        A list of Disk device types and bus type for HDD's
        e.g., ['SSD', 'SSD', 'HDD', 'SAS', 'HDD', 'SATA']
    '''
    cmdpath = controller_tool.lower()
    if 'storcli' in cmdpath or 'perccli' in cmdpath:
        return disk_type_with_stor_cli (controller_tool)
    elif 'arcconf' in cmdpath:
        return disk_type_with_arcconf (controller_tool)
    elif 'sas2ircu' in cmdpath:
        return disk_type_with_sas2ircu (controller_tool)
    else:
        print ( "Error: Unknown controller tool %s " % controller_tool )
        sys.exit(1)

def add_disk_to_list(type, bus, return_list):
    if type == Disk.DSK_TYP_SSD:
        return_list.append(Disk.DSK_TYP_SSD)
    else:
        if type != Disk.DSK_TYP_HDD:
            print "Warning: Unknown disk type %s" % type
        return_list.append(Disk.DSK_TYP_HDD)
        if bus == Disk.DISK_BUS_SAS:
            return_list.append(Disk.DISK_BUS_SAS)
        elif bus == Disk.DISK_BUS_SATA:
            return_list.append(Disk.DISK_BUS_SATA)
        else:
            return_list.append(Disk.DISK_BUS_NA)

def disk_type_with_sas2ircu (sas2ircu):
    '''
    This issues an sas2ircu command; for description of inputs and outputs see disk_type_with_controller_tool
    '''
    return_list = []

    controllers = []

    # Regular expressions to find particular details needed from sas2ircu output
    controller_index = re.compile("[0-9]+")
    device_section = re.compile("Device is a Hard disk")
    state_line = re.compile("State\s+:\s*Ready")
    drive_type_line = re.compile("Drive Type\s+:\s*(S[A-Z]+)_([A-Z]+)\s")

    # sas2ircu to list controller numbers
    output = subprocess.Popen([sas2ircu, "LIST" ], stdout=subprocess.PIPE).stdout 
    #             Adapter      Vendor  Device                       SubSys  SubSys 
    #    Index    Type          ID      ID    Pci Address          Ven ID  Dev ID 
    #    -----  ------------  ------  ------  -----------------    ------  ------ 
    #        0     SAS2004     1000h    70h   00h:01h:00h:00h      1000h   3010h 
 # itemindex 0           1         2      3                 4          5       6

    found_controller_index = False
    reading_controller_indices = False
    for line in output:
        items = line.strip ('\r\n').split()
        if len(items) > 0:
            found_controller_index = controller_index.match(items[0])
            if not reading_controller_indices and found_controller_index and len(items) == 7:
                reading_controller_indices = True
            elif reading_controller_indices:
                break # done reading the controller table
            if reading_controller_indices:
                controllers.append(items[0])

    for controller in controllers:
        # sas2ircu arguments
        # DISPLAY = the actual command being issued
        # controller = controller to check
        output = subprocess.Popen([sas2ircu, controller, "DISPLAY"], stdout=subprocess.PIPE).stdout

        state_line_found = False
        drive_type_line_found = False

        for line in output:
           device_section_found = device_section.match(line)
           if device_section_found:
                # from this point on gather info about a device
                # if we found a qualified device before, record the info
                if state_line_found:
                    add_disk_to_list(type, bus, return_list)
                # initialize all variables for the new round (new device)
                state_line_found = False
                drive_type_line_found = False
                type = None
                bus = Disk.DISK_BUS_NA
           else: # we are in device section parsing mode
               if not state_line_found:
                   state_line_found = state_line.search(line)
               if not drive_type_line_found:
                   drive_type_line_found = drive_type_line.search(line)
                   if drive_type_line_found:
                       bus = drive_type_line.search(line).group(1)
                       type = drive_type_line.search(line).group(2)
        # we are done with this contoller, let's record the last disk if there is one
        if state_line_found:
            add_disk_to_list(type, bus, return_list)
    return return_list

def disk_type_with_arcconf (arcconf):
    '''
    This issues an arcconf command; for description of inputs and outputs see disk_type_with_controller_tool
    '''

    return_list = []
    
    # Regular expressions to find particular details needed from arcconf output
    controller_number_section = re.compile ("Controllers found: ([0-9]*)")
    device_section = re.compile("Device #")
    state_line = re.compile("State\s+:\s*Online")
    ssd_line = re.compile("SSD\s+:")
    transfer_line = re.compile("Transfer Speed\s+:\s*(S[A-Z]+)\s")

    controller_number = 1
    found_controller_number_section = False
    num_controllers = 0

    while True: # keep calling the command until we run out of controllers, then break out of the loop 
        # arcconf arguments
        # GETCONFIG = the actual command being issued
        # controller_number = controller to check
        # PD = only check physical devices
        output = subprocess.Popen([arcconf, "GETCONFIG", str(controller_number), "PD" ], stdout=subprocess.PIPE).stdout

        state_line_found = False
        ssd_line_found = False
        transfer_line_found = False
 
        for line in output:
            if not found_controller_number_section:
                found_controller_number_section = controller_number_section.match(line) 
                if found_controller_number_section:
                    num_controllers = int(controller_number_section.match(line).group(1))
                    if num_controllers == 0:
                        print ( "Error: no controllers found" )
                        sys.exit(1)
                    continue
            device_section_found = device_section.search(line)
            if device_section_found:
                # from this point on gather info about a device
                # if we found a qualified device before, record the info
                if state_line_found:
                    add_disk_to_list(type, transfer, return_list)
                # initialize all variables for the new round (new device)
                state_line_found = False
                ssd_line_found = False
                type = Disk.DSK_TYP_HDD
                transfer_line_found = False
                transfer = Disk.DISK_BUS_NA
            else: # we are in device section parsing mode
                if not state_line_found: 
                    state_line_found = state_line.search(line)
                if not ssd_line_found:
                    ssd_line_found = ssd_line.search(line)
                    if ssd_line_found and "Yes" in line:
                        type = Disk.DSK_TYP_SSD 
                if not transfer_line_found:
                    transfer_line_found = transfer_line.search(line)
                    if transfer_line_found:
                        transfer = transfer_line.search(line).group(1)
        
        # we are done with this contoller, let's record the last disk if there is one
        if state_line_found:
            add_disk_to_list(type, transfer, return_list)

        controller_number = controller_number + 1            
        if controller_number > num_controllers:
            break
    return return_list

def disk_type_with_stor_cli (stor_client):
    '''
    This issues a storcli/perccli command; for description of inputs and outputs see disk_type_with_controller_tool
    notes:
        This may need to be expanded in the future to limit commands to certain
        controllers or enclosures.
    '''

    # Regular expressions to find particular details needed from storcli64 output

    # don't bother looking for drive info until we find the drive info section
    drive_info_section = re.compile ("^Drive Information :$")

    # Only look for online drives, others are not exposed to the OS
    online_drive_info = re.compile (".* (Onln|JBOD) .* (HDD|SSD) .*")

    found_drive_info = False
    return_list = []

    # storcli64/perccli64 arguments:
    #    /call = check all controllers
    #    /eall = check all enclosures (megaraid sas card concept)
    #    /sall = check all slots (aka physical devices, e.g., drives)
    #    show  = the actual command being issued
    output = subprocess.Popen([stor_client, "/call/eall/sall", "show"], stdout=subprocess.PIPE).stdout

    last_drive_group = 99999999
    drive_group = re.compile("[0-9]")

    for line in output:
        # Look for beginning of drive information section in output
        if not found_drive_info:
            drive_info_section_found = drive_info_section.match (line)
            if drive_info_section_found:
                dbg_print (line.rstrip())
                found_drive_info = True
            continue
        drive_found = online_drive_info.match(line)
        if drive_found:
            # EID:Slt DID State DG      Size Intf Med SED PI SeSz Model             Sp
            # 245:0    26 Onln   0 465.25 GB SATA HDD N   N  512B ST500DM002-1BD142 U
   # itemindex 0      1   2     3       4 5  6    7   8   9  10   11                12
            items = line.strip ('\r\n').split()

            # check for hardware raid array
            if drive_group.match(items[3]):  # check if drive group is applicable (n/a for JBOD)
                if last_drive_group == items[3]:
                    continue
                last_drive_group = items[3]

            if Disk.DSK_TYP_HDD == items[7]:
                return_list.append(Disk.DSK_TYP_HDD)
                if Disk.DISK_BUS_SAS == items[6]:
                    return_list.append (Disk.DISK_BUS_SAS)
                    continue
                if Disk.DISK_BUS_SATA == items[6]:
                    return_list.append (Disk.DISK_BUS_SATA)
                    continue
                print "Unexpected interface type found in:  ", line
                sys.exit(1)

            if Disk.DSK_TYP_SSD == items[7]:
                return_list.append(Disk.DSK_TYP_SSD)
                continue
            # This should never happen, but...
            print "Unexpected entity found:  ", line
            sys.exit(1)

    dbg_print ("return_list=" + ', '.join(return_list))
    return return_list

def find_index_devices(index_mount_point) :
    devnull = open(os.devnull, 'wb')
    output = subprocess.Popen(["df", index_mount_point], stdout=subprocess.PIPE, stderr=devnull).stdout
    index_devs = []
    dev = ""
    for line in output:
        items = line.strip ('\r\n').split()
        if index_mount_point == items[5]:
            dev = items[0]
            dbg_print("Found an existing index device in df output: %s" % dev)
            break

    if 0 != len(dev):
        index_devs = get_device_list(dev)
    return index_devs

if __name__ == "__main__":

    # Parse command line options
    parser = optparse.OptionParser("usage: %prog [options]")

    parser.add_option('-D', '--debug', dest = 'debug', action = 'store_true', help = 'Turn on debugging')
    parser.add_option('-f', '--fds-root', dest = 'fds_root', default='/fds', help = 'Path to fds-root')
    parser.add_option('-s', '--stor', dest = 'stor_cli', help = 'Full path and file name of the StorCli/PercCli binary')
    parser.add_option('-v', '--virtual', dest = 'virtual', action = 'store_true', help = 'Running in a virtualized environment, treats all detected drives as HDDs')
    parser.add_option('-w', '--write', dest = 'write_disk', action = 'store_true', help = 'Writes the disk configuration information file.')
    parser.add_option('-m', '--map', dest = 'disk_config_dest', help = 'The destination disk config file.')

    (options, args)   = parser.parse_args()
    debug_on          = options.debug
    write_disk_config = options.write_disk

    # verify UID=0
    if os.getuid() != 0:
        print ( "Error:  must be run as root.  Can not continue.")
        sys.exit(1)

    # verify stor_cli setting is good
    if options.stor_cli:
        if not os.access (options.stor_cli, os.X_OK):
            print ( "Error:  Can not access '" + options.stor_cli + "', please use or verify the -s command line option.  Can not continue.")
            sys.exit(1)

    fds_root = options.fds_root
    if not fds_root.endswith ('/'):
        fds_root += '/'
    if not options.disk_config_dest:
        destination_dir = fds_root + "dev"
    else:
        destination_dir = options.disk_config_dest
    index_mount_point = fds_root + "sys-repo"

    # verify destination directory exists
    if write_disk_config:
        if not os.path.isdir (destination_dir):
            print ( "Error:  Directory '" + destination_dir + "', does not exist.  Can not continue.")
            sys.exit(1)
    elif options.disk_config_dest:
        print ( "Error: must specify -m(--map) option together with the -w (--write) option.")
        sys.exit(1)
    
    
    os_device_list = discover_os_devices()

    sys.stdout.write ('Scanning hardware:  Phase 1:  ')
    sys.stdout.flush()
    dbg_print ('')

    dev_list = Disk.sys_disks (options.virtual, os_device_list)

    sys.stdout.write ("Complete ")
    sys.stdout.flush()

    controller_disk_list = []   # List of disk types attached via add-in controllers

    if options.stor_cli:
        for disk in dev_list:
            sys.stdout.write ("  Phase 2:   ")
            sys.stdout.flush()
            dbg_print ('')

            controller_disk_list = disk_type_with_controller_tool(options.stor_cli)

            sys.stdout.write ("Complete")
            sys.stdout.flush()

            break
    print ''

    dbg_print ("controller_disk_list = " + ', '.join(controller_disk_list))

    for disk in dev_list:
        print disk.get_type(), disk.get_path()
        if len (controller_disk_list) > 0 and disk.target:  # don't try to use controller info for internal disks
            disk.dsk_typ = controller_disk_list.pop(0)
            if disk.dsk_typ == Disk.DSK_TYP_HDD:
                disk.set_bus (controller_disk_list.pop(0))
        elif disk.get_type() == Disk.DSK_TYP_UNKNOWN:
            # devices exist in /dev/sd* that can't be identified.
            print ( "Error:  Identified but unknown type devices remain.  Try using -s perhaps?  Can not continue.")
            #debug_dump (dev_list)
            sys.exit(1)

    # no more devices are known in /dev/sd*, but we have more drives documented on hardware controllers
    if len (controller_disk_list) > 0:
        print ( "Error:  Unexpected devices remain identified.  Are raid devices present on controller cards?  Can not continue.")
        #debug_dump (dev_list)
        sys.exit(1)

    # Now figure out which drives will hold the meta data indexes
    # Possible scenarios
    #     3 SSDs (excluding the os drives), medium and large systems
    #     2 SSDs (excluding the OS drives and only 2 present SSDs, on small systems
    #     only SSD and first HDD, Odd hardware combination?
    #     first 2 HDDs, soak clusters and VMs
    #
    ssd_device_list = []
    hdd_device_list = []

    for disk in dev_list:
        if True == disk.get_os_use():
            continue
        if Disk.DSK_TYP_SSD == disk.get_type():
            ssd_device_list.append (disk.get_path())
            if 3 == len(ssd_device_list):
                break
        else:
            if len(hdd_device_list) < 2:
                hdd_device_list.append (disk.get_path())

    if len (ssd_device_list) + len (hdd_device_list) < 2:
        print "Error:  At least 2 devices usable as data storage must be present.  Found %d device(s).  Can not continue." % (len (ssd_device_list) + len (hdd_device_list))
        sys.exit(1)

    # reverse these two lists -- pop() works from the tail of the list
    ssd_device_list.reverse()
    hdd_device_list.reverse()

    index_device_list = []

    # keep existing index disk(s)
    index_device_list = find_index_devices(index_mount_point)
    if len(index_device_list) < 1:
        devnull = open(os.devnull, 'wb')
        subprocess.call(['mount', index_mount_point], stdout = None, stderr = devnull)
        index_device_list = find_index_devices(index_mount_point)

    # copy one SSD into the index device list
    while len (index_device_list) < 1 and len (ssd_device_list) > 0:
        index_device_list.append (ssd_device_list.pop())

    # add enough HDDs to ensure one drive is marked as an index
    while len (index_device_list) < 1:
        index_device_list.append (hdd_device_list.pop())

    for disk in dev_list:
        if disk.get_path() in index_device_list:
            disk.set_index()

    if write_disk_config:
        hints_file = destination_dir + '/disk-config.conf'
        file_on_disk = open (hints_file, 'w')
        for disk in dev_list:
            disk.print_disk(file_on_disk)
        file_on_disk.close()
        print "The disk config has been written to ", hints_file
    else:  # store the disk.hints map
        print "===================================================================================================="
        print "This disk configuration is NOT saved, this would be written if -w was included on the comnmand line."
        print "===================================================================================================="
        for disk in dev_list:
            disk.print_disk()
