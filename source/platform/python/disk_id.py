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
    dsk_prefix = '/dev/'

    dsk_typ_ssd = 'SSD'
    dsk_typ_hdd = 'HDD'
    dsk_typ_unknown = 'UKN'

    p_dev   = re.compile('sd[a-z]$')

    # We are using 1000*1000
    # device size with M = 1024*1024:     1907729 MBytes
    # device size with M = 1000*1000:     2000398 MBytes (2000 GB)
    dsk_gsize = 1000 * 1000 * 1000

    # Save lshw result once
    dsk_lshw_xml  = None

## Public member functions
## -----------------------
    # Parse the disk information
    def __init__(self, path, virtualized):
        assert path != None
        assert re.match(Disk.dsk_prefix, path)

        self.dsk_path = path
        self.dsk_cap  = 0
        self.dsk_typ  = Disk.dsk_typ_unknown
        self.dsk_formatted = 'Unknown'

        # parse disk information
        self.__parse_with_lshw(path, virtualized)

    # drive type: HDD|SSD
    def get_type(self):
        return self.dsk_typ

    # disk capacity in GB
    def get_capacity(self):
        return self.dsk_cap

    # file system path of disk
    def get_path(self):
        return self.dsk_path

    # print all the parsed fields
    def print_disk(self, disk_file = None):
        global header_output
        dest = sys.stdout

        if disk_file is not None:
            dest = disk_file

        if not header_output:
            print >>dest, '#path     type  capacity (GB)'
            header_output = True
        print >>dest, '%-10s%-5s%-5d' % (self.get_path(), self.get_type(), self.get_capacity())

## ----------------------------------------------------------------

    # get all Disk objects in the system, including the boot device(s)
    @staticmethod
    def sys_disks(virtualized):
        dev_list  = []
        path_list = Disk.__get_sys_disks_path()
    
        for path in path_list:
            disk = Disk(path, virtualized)
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
            if node.get('id') == 'cdrom':
                continue
            node_logicalname = node.find('logicalname')
            assert node_logicalname != None

            if node_logicalname.text != path:
                continue

            node_size = node.find('size')
            assert node_size != None

            units = node_size.get('units')
            if units == 'bytes':
                self.dsk_cap = int(node_size.text) / Disk.dsk_gsize
                break
            else:
                self.dsk_cap = 0
                print 'ERROR: lshw units size not implemented'
                assert False
        assert self.dsk_cap != 0

        # match type
        dev = re.split('/', path)
        assert len(dev) == 3
        fp = open('/sys/block/' + dev[2] + '/queue/rotational', 'r')
        rotation = fp.read().strip()
        fp.close()

        if rotation == '0':                    # SSD
            self.dsk_typ = Disk.dsk_typ_ssd
        elif rotation > '6000':                # HDD
            self.dsk_typ = Disk.dsk_typ_hdd
        elif virtualized != False:             # Virtualized treated as HDD's
            self.dsk_typ = Disk.dsk_typ_hdd
                                               # else this device is on a hardware controller, 
                                               # so leave the dsk_typ set to the default UKN (unknown)

    @staticmethod
    def __get_sys_disks_path():
        path_info = subprocess.Popen(['ls', Disk.dsk_prefix], stdout=subprocess.PIPE).stdout
        path_list = []
        for rec in path_info:
            dsk = Disk.p_dev.match(rec)
            if dsk:
                dbg_print('Found Disk: ' + Disk.dsk_prefix + dsk.group(0))
                path_list.append(Disk.dsk_prefix + dsk.group(0))
        return path_list

## -------------------------------------------------------------------------------

def dbg_print(msg):
    if debug_on:
        if len (msg) > 0:
            print 'Debug: ' + msg
        else:
            print msg

def disk_type_with_stor_cli (stor_client):
    '''
    This uses the megaraid storcli64 tool to inspect the raid controllers and enclosures
    to find storage media types attached to hardware controllers.

    inputs:
        stor_client:  Full path and binary name of the storcli64 tool

    returns:
        A list of class Disk device types, e.g., ['SSD', 'SSD', 'HDD', 'HDD']

    notes:
        This may need to be expanded in the future to limit commands to certain
        controllers or enclosures.
    '''

    # Regular expressions to find the particular details needed from storcli64 output

    # don't bother looking for drive info until we find the drive info section
    drive_info_section = re.compile ("^Drive Information :$")

    # Only look for online drives, others are not exposed to the OS
    online_drive_info = re.compile (".* Onln .* (HDD|SSD) .*")

    # Once an online device is found, determine the type
    check_for_SSD = re.compile (".* SSD .*")
    check_for_HDD = re.compile (".* HDD .*")

    found_drive_info = False
    return_list = []

    # storcli64 arguments:
    #    /call = check all controllers
    #    /eall = check all enclosures (megaraid sas card concept)
    #    /sall = check all slots (aka physical devices, e.g., drives)
    #    show  = the actual command being issued
    output = subprocess.Popen([stor_client, "/call/eall/sall", "show"], stdout=subprocess.PIPE).stdout

    for line in output:
        if not found_drive_info:
            drive_info_section_found = drive_info_section.match (line)
            if drive_info_section_found:
                dbg_print (line.rstrip())
                found_drive_info = True
            continue
        drive_found = online_drive_info.match(line)
        if drive_found:
            if check_for_HDD.match(line):
                return_list.append(Disk.dsk_typ_hdd)
                continue
            if check_for_SSD.match(line):
                return_list.append(Disk.dsk_typ_ssd)
                continue
            # This should never happen, but...
            print "Unexpected entity found:  ", line
            sys.exit(1)

    dbg_print ("return_list=" + ', '.join(return_list))
    return return_list

if __name__ == "__main__":

    # Parse command line options
    parser = optparse.OptionParser("usage: %prog [options]")

    parser.add_option('-f', '--fds-root', dest = 'fds_root', default='/fds', help = 'Path to fds-root')
    parser.add_option('-s', '--stor', dest = 'stor_cli', default='/usr/local/MegaRAID Storage Manager/StorCLI/storcli64', help = 'Full path and file name of the StorCli binary')
    parser.add_option('-p', '--print', dest = 'print_disk', action = 'store_true', help = 'Display the disk information on screen and do not write to the config file')
    parser.add_option('-D', '--debug', dest = 'debug', action = 'store_true', help = 'Turn on debugging')
    parser.add_option('-v', '--virtual', dest = 'virtual', action = 'store_true', help = 'Running in a virtualized environment (Treats all detected drives as HDD), also disables the check for storcli64')

    (options, args) = parser.parse_args()
    debug_on     = options.debug
    print_disk   = options.print_disk

    # verify UID=0
    if os.getuid() != 0:
        print ( "Error:  must be run as root.  Can not continue.")
        sys.exit(1)

    # verify stor_cli setting is good
    if not options.virtual:
        if not os.access (options.stor_cli, os.X_OK):
            print ( "Error:  Can not access '" + options.stor_cli + "', please use or verify the -s command line option.  Can not continue.")
            sys.exit(1)

    destination_dir = options.fds_root + "/dev"

    # verify destination directory exists
    if not os.path.isdir (destination_dir):
        print ( "Error:  Directory '" + destination_dir + "', does not exist.  Can not continue.")
        sys.exit(1)

    sys.stdout.write ('Scanning hardware:  Phase 1:  ')
    sys.stdout.flush()
    dbg_print ('')

    dev_list = Disk.sys_disks(options.virtual)

    sys.stdout.write ("Complete")
    sys.stdout.flush()

    controller_disk_list = []   # List of disk types attached via add-in controllers

    for disk in dev_list:
        if disk.get_type() == Disk.dsk_typ_unknown:
            sys.stdout.write ("  Phase 2:   ")
            sys.stdout.flush()
            dbg_print ('')

            controller_disk_list = disk_type_with_stor_cli(options.stor_cli)

            sys.stdout.write ("Complete")
            sys.stdout.flush()

            break;
    print ''

    dbg_print ("controller_disk_list = " + ', '.join(controller_disk_list))

    for disk in dev_list:
        print "disk.get_type() = ", disk.get_type()
        if disk.get_type() == Disk.dsk_typ_unknown:
            if len(controller_disk_list) > 0:
                disk.dsk_typ = controller_disk_list.pop(0)
            else:
                # devices exist in /dev/sd* that can't be identified.
                print ( "Error:  Identified but unknown type devices remain, but are not known to controller cards.  Can not continue.")
                sys.exit(1)

    # no more devices are known in /dev/sd*, but we have more drives documented on hardware controllers
    if len (controller_disk_list) > 0:
        print ( "Error:  Unexpected devices remain identified.  Are raid devices present on controller cards?  Can not continue.")
        #### DJN ADD THIS BACK
        #sys.exit(1)

    if print_disk:
        print "=============================================================================================="
        print "This disk list is NOT saved, this is what would be written if -p was not on the comnmand line."
        print "=============================================================================================="
        for disk in dev_list:
            disk.print_disk()
    else:  # store the disk.hints map
        hints_file = destination_dir + '/disk-hints.conf'
        file_on_disk = open (hints_file, 'w')  
        for disk in dev_list:
            disk.print_disk(file_on_disk)
        file_on_disk.close()
        print "Disk type hints have been written to ", hints_file
