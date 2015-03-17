#!/usr/bin/env python

import shutil
import time
import distutils.util
import copy

import fstab        # Installed via pypi or omnibus

import sys
import os
import optparse
import subprocess
import math

# TODO(donavan)
#  Global todo list
# 1)  Support device filtering --device option
# 2)  Support a dynamicly configured fstab, backups of the real fstab are created
# 3)  Overriding --fds-root breaks numerous pseudo python constants

# Some defaults
DEFAULT_FDS_ROOT = '/fds/'
DEFAULT_SHORT_PATH_AND_HINTS = 'dev/disk-config.conf'
FSTAB_PATH_AND_FILENAME = "/etc/fstab"

# Thes semi-arbitrary looking numbers are documented in the google doc titled:  "DM and SM Partition Size"
DM_INDEX_BYTES_PER_OBJECT = 32
SM_INDEX_BYTES_PER_OBJECT = 603

OBJECT_SIZE = 16384

HIGH_INDEX_MODE_VALUE = 0.25     # When to use the storage space on the index drives in capacity calculations

##### Some constants used when creating partitions
PARTITION_START_MB = 2
                           # f   o   r   m   a   t   i   o   n       D   a   t   a       S   y   s   t   e   m   s             D   I   S   K
DISK_MARKER = bytearray ('\x46\x6f\x72\x6d\x61\x74\x69\x6f\x6e\x00\x44\x61\x74\x61\x00\x53\x79\x73\x74\x65\x6d\x73\x00\xfd\x00\x44\x49\x53\x4b\x00')
DISK_MARKER_LEN = len (DISK_MARKER)

FDS_SUPERBLOCK_SIZE_IN_MB = 10

PARITION_TYPE = "xfs"

DM_INDEX_MOUNT_POINT = DEFAULT_FDS_ROOT + "sys-repo/dm"
SM_INDEX_MOUNT_POINT = DEFAULT_FDS_ROOT + "sys-repo/sm"

BASE_SSD_MOUNT_POINT = DEFAULT_FDS_ROOT + "dev/ssd-"
BASE_HDD_MOUNT_POINT = DEFAULT_FDS_ROOT + "dev/hdd-"

MOUNT_OPTIONS = "noauto"           # mount defaults still apply
WHITE_SPACE = "   "                # used to pad fstab lines

# Some General Constants
MD_COUNT_RANGE_END = 128

# Turn on debugging
global_debug_on = False

# Header displayed
header_output = False

class extendedFstab (fstab.Fstab):

    def __init__ (self):
        self.altered = False


    def remove_mount_point_by_uuid (self, uuid):
        lines_to_keep = []
        removed = False
        for line in self.lines:
            if not removed and uuid in line.get_raw():
                removed = True
            else:
                lines_to_keep.append (line)
        if removed:
            self.lines = lines_to_keep
            self.altered = True


    def add_mount_point (self, line):
        self.lines.append (fstab.Line (line + '\n'))
        self.altered = True


    def backup_if_altered (self, fstab_file):
        if self.altered:
            shutil.copyfile (fstab_file, fstab_file + '.' + str (time.time()).rstrip ('1234567890').rstrip ('.'))
            return True
        return False


class Base (object):

    def call_subproc (self, call_list):
        self.dbg_print_list (call_list)
        res = subprocess.call (call_list, stdout = None, stderr = None)

        if res != 0:
            self.system_exit ('')


    def dbg_print (self, msg):
        if global_debug_on:
            print 'Debug: ' + msg
    

    def dbg_print_list (self, list):
        if global_debug_on:
            print 'Debug: ',
            print list
    

    def system_exit (self, msg):
        if len (msg) > 0:
            print "Fatal Error:  " + msg
        sys.exit (8)


class BaseDisk (object):

    def get_uuid (self, device):
        call_list = ['blkid', '-s', 'UUID', '-o', 'value', device]
        output = subprocess.Popen (call_list, stdout=subprocess.PIPE).stdout

        return output.read().strip ('\r\n')


class Disk (Base):

    DISK_TYPE_SSD = "SSD"
    DISK_TYPE_HDD = "HDD"

    SAS_INTERFACE = "SAS"

    SSD_ID = "SS"         # Disk identified as SSD Last 2 bytes of FDS disk marker (magic)
    HDD_SATA = "HT"       # Disk identified as HDD on SATA interface
    HDD_SAS = "HS"        # Disk identified as HDD on SAS interface
                          # Disks identified as HDD and interface NA (on VMs) are treated as "HT"
    def __init__ (self, path, os_disk, index_disk, disk_type, interface, capacity):
        self.path = path
        self.os_disk = os_disk
        self.index_disk = index_disk
        self.interface = interface
        self.capacity = capacity
        self.sm_flag = False
        self.dm_flag = False

        if Disk.DISK_TYPE_SSD == disk_type:
            self.disk_type = Disk.DISK_TYPE_SSD
            self.disk_marker_id = Disk.SSD_ID
        elif Disk.DISK_TYPE_HDD == disk_type:
            self.disk_type = Disk.DISK_TYPE_HDD
            if Disk.SAS_INTERFACE == self.interface:
                self.disk_marker_id = Disk.HDD_SAS
            else:
                self.disk_marker_id = Disk.HDD_SATA
        else:
            self.system_exit ('Invalid disk type encountered:  ' + disk_type)

        self.load_disk_marker()


    def get_path (self):
        return self.path


    def get_type (self):
        return self.disk_type


    def get_capacity (self):
        return self.capacity


    def get_index (self):
        return self.index_disk


    def get_os_usage (self):
        return self.os_disk


    def set_sm_flag (self):
        self.sm_flag = True


    def set_dm_flag (self):
        self.dm_flag = True


    def check_for_fds (self):
        if self.os_disk:
            return False
        if DISK_MARKER == self.marker:
            print ('Device (%s) is already FDS formatted.' % (self.path))
            return True
        return False


    def partition (self, dm_size, sm_size):
        self.dbg_print ("Evaluating:  %s" % (self.path))

        if self.os_disk:
            return

        self.dbg_print ("Ready to partition %s as a Formation device." % (self.path))

        # Laydown a GPT disk header, this blows away any existing parition information
        call_list = ['parted', '--script', self.path, 'mklabel', 'gpt']
        self.call_subproc (call_list)

        # Create the super block
        self.dbg_print ("\tCreating FDS SuperBlock Partition")
        part_start = PARTITION_START_MB
        part_end = part_start + FDS_SUPERBLOCK_SIZE_IN_MB
        call_list = ['parted', '--script', '--align', 'optimal', self.path, 'mkpart', 'formation_superblock', PARITION_TYPE, str (part_start) + 'MB', str (part_end) + 'MB', 'set', '1', 'hidden', 'on']
        self.call_subproc (call_list)

        # if this device is an index device, create partition 2
        if self.index_disk:
            self.dbg_print ("\tCreating FDS Index Partition")
            part_start = part_end

            if self.dm_flag:
                part_end = part_start + dm_size
            elif self.sm_flag:
                part_end = part_start + sm_size
            else:
                self.system_exit ('Found an Index disk without a dm or sm flag set. ')

            call_list = ['parted', '--script', '--align', 'optimal', self.path, 'mkpart', 'formation_index', PARITION_TYPE, str (part_start) + 'MB', str (part_end) + 'MB']
            self.call_subproc (call_list)

        # Create the Data partition
        self.dbg_print ("\tCreating FDS Data Partition")
        call_list = ['parted', '--script', '--align', 'optimal', self.path, 'mkpart', 'formation_data', str (part_end) + 'MB', '\"-1\"']
        self.call_subproc (call_list)


    def load_disk_marker (self):

        if self.os_disk:
            return

        # Try to read the disk marker from partion 1.  If it fails, we presume this is not an FDS disk.
        try:
            file_handle = open (self.path + '1', "rb")
        except:
            return

        self.marker = bytearray (file_handle.read (DISK_MARKER_LEN))
        file_handle.close()


    def format (self):
        ''' TODO(donavan) This needs to change to format parition 1 on OS disks
            But for now until every dev system is partitioned using the standard
            partitioning scheme, this is not possible.
            Also need to fix load_disk_header() '''

        if self.os_disk:
            return

        self.dbg_print ("Formatting:  %s" % (self.path))

        # Write the FDS disk marker and Zero out the FDS superblock partition
        superblock_partition = self.path + '1'

        call_list = ['blockdev', '--getsize64', superblock_partition]
        output = subprocess.Popen (call_list, stdout=subprocess.PIPE).stdout
        partition_size = output.read().strip ('\r\n')

        zero_buffer = bytearray (int (partition_size) - DISK_MARKER_LEN - 2)   # 2 bytes are for the marker_id

        file_handle = open (superblock_partition, "wb")
        file_handle.write (DISK_MARKER)
        file_handle.write (self.disk_marker_id)
        file_handle.write (zero_buffer)
        file_handle.close()

        # format the remaining partions
        if self.index_disk:
            partition_list = ['2', '3']
        else:
            partition_list = ['2']

        for partition in partition_list:
            call_list = ['mkfs.' + PARITION_TYPE, self.path + partition, '-f', '-q']
            self.call_subproc (call_list)


    def print_disk (self):
        ''' print all the parsed fields '''

        global header_output

        if not header_output:
            print '#path      os_use  index_use  type  interface  capacity (GB)'
            header_output = True
        print '%-11s%-8s%-11s%-12s%-6s%-s' % (self.path, self.os_disk, self.index_disk, self.disk_type, self.interface, self.capacity)


    def dump (self):
        self.dbg_print ("%s %s %s %s %s" % (self.path, self.os_disk, self.index_disk, self.disk_type, self.capacity))


class RaidDevice (Base, BaseDisk):

    def reset_raid_components (self, raid_device, raid_partitions):
        self.dbg_print ("Removing raid_device:  %s" % (raid_device))
    
        call_list = ['mdadm', '--stop', '--quiet', raid_device]
        self.call_subproc (call_list)
    
        call_list = ['mdadm', '--zero-superblock']
        for part in raid_partitions:
            call_list.append (part)
        self.call_subproc (call_list)
    
    
    def cleanup_raid_if_in_use (self, partition_list, fstab, disk_utils):
        # Check all existing raid arrays to see if any items in partition_list are in use as part of a raid array
        self.dbg_print ("Checking for raid arrays")
        for id in range (MD_COUNT_RANGE_END):
            md_test_dev = '/dev/md' + str (id)
            if os.path.exists (md_test_dev):
                self.dbg_print ("    found raid arrays:  " + md_test_dev)
                call_list = ['mdadm', '--detail', md_test_dev]
                output = subprocess.Popen (call_list, stdout=subprocess.PIPE).stdout
                for line in output:
                    for part in partition_list:
                        if part in line:
                            # Should force use of a --reformat type CLI here.
                            self.dbg_print ("    found raid component:  " + part)
                            mounts = disk_utils.find_mounts (md_test_dev)
                            for mount in mounts:
                                call_list = ['umount', '-f', mount]
                                call_subproc (call_list)
                            file_sys_uuid = self.get_uuid (md_test_dev)
                            fstab.remove_mount_point_by_uuid (file_sys_uuid)
                            self.reset_raid_components (md_test_dev, partition_list)
                            return True
        return False
    
    
    def create_index_raid (self, partition_list):
        if len (partition_list) < 2:
            return None
    
        md_dev = None
    
        #find next free /dev/mdX device
        for id in range (MD_COUNT_RANGE_END):
            md_test_dev = '/dev/md' + str (id)
            if os.path.exists (md_test_dev):
                continue
            md_dev = md_test_dev
            break;
    
        if md_dev is None:
            self.system_exit ('Unable to find a suitable /dev/md slot');
    
        output = ""
        # Build printable partition list
        for part in partition_list:
            output += " "
            output += part
    
        self.dbg_print ("Preparing to assemble raid array %s using %s" % (md_dev, output))
    
        # Assemble a raid array from partition 2 on each device
        call_list = ['mdadm', '--create', '--quiet', '--metadata=1.2', md_dev, '--level=0', '--raid-devices=2']
        for part in partition_list:
            call_list.append (part);
    
        self.call_subproc (call_list)
    
        # Create the array
        call_list = ['mkfs.' + PARITION_TYPE, md_dev, '-f', '-q']
        self.call_subproc (call_list)
    
        return self.get_uuid (md_dev)


class DiskUtils (Base, BaseDisk):

    def find_mounts (self, device):
        mount_points = []
        input_file = open ('/proc/mounts', 'r')
    
        for line in input_file:
            if device in line:
                items = line.strip ('\r\n').split()
                mount_points.append (items[1])
    
        self.dbg_print_list (mount_points)
    
        return mount_points

    
    def cleanup_mounted_file_systems (self, fstab, partition_list):
        ''' clean up mounted file systems '''
        for part in partition_list:
            mounts = self.find_mounts (part)
            for mount in mounts:
                call_list = ['umount', '-f', mount]
                self.call_subproc (call_list)
            fstab.remove_mount_point_by_uuid (self.get_uuid (part))

    
    def find_disk_type (self, disk_list, part):
        disk_path = part.rstrip ('1234567890')
        for disk in disk_list:
            if disk.get_path() == disk_path:
                return disk.get_type()
        self.system_exit ('Unable to determine disk type for ' + disk_path);


class DiskManager (Base):

    def __init__ (self):

        self.print_disk = False
        self.disk_config_file = None

        self.disk_list = []
        self.dm_index_MB = 0
        self.sm_index_MB = 0

        self.fstab = extendedFstab()

        # Partition lists
        self.sm_index_partition_list = []         # Storage manager index partitions
        self.dm_index_partition_list = []         # Data manager index partitions
        self.data_partition_list = []             # Object storage paritions

        self.umount_list = []                     # List of partitions that may need to be unmounted

        self.raid_manager = None
        self.disk_utils = DiskUtils()


    def process_command_line (self):
        parser = optparse.OptionParser ("usage: %prog [options]")
    
        # need to flesh out support for --device feature, the filtering works, but we shouldn't be trying to create index devices with a filtered device
        parser.add_option ('-d', '--device', dest = 'device', help = 'Limit operation to listed device (e.g. /dev/sdf)')
        parser.add_option ('-f', '--fds-root', dest = 'fds_root', default=DEFAULT_FDS_ROOT, help = 'Path to fds-root')
        parser.add_option ('--format', dest = 'format', action= 'store_true', help = 'Format disks for use by FDS.  If previously formatted, use --reset.')
        parser.add_option ('--fstab', dest = 'fstab', default = FSTAB_PATH_AND_FILENAME, help = 'Operate on supplied fstab file rather than ' + FSTAB_PATH_AND_FILENAME)
        parser.add_option ('-m', '--map', dest = 'disk_config_file', default=DEFAULT_SHORT_PATH_AND_HINTS, help = 'The disk config file generated by disk_id.py.  Default = ' + DEFAULT_FDS_ROOT + DEFAULT_SHORT_PATH_AND_HINTS )
        parser.add_option ('-r', '--reset', dest = 'reset', action = 'store_true', help = 'Reset all storage space, requires FDS to be stopped.')
        parser.add_option ('-p', '--print', dest = 'print_disk', action = 'store_true', help = 'Print disk information')
        parser.add_option ('-D', '--debug', dest = 'debug', action = 'store_true', help = 'Turn on debugging')
    
        (self.options, self.args) = parser.parse_args()
    
        # verify UID=0
        if os.getuid() != 0:
            self.system_exit ("Must be run as root.  Can not continue.")
    
        if self.options.device:
            self.system_exit ("Device filtering is not ready for use.  Can not continue.")
    
        global_debug_on = self.options.debug
        self.print_disk = self.options.print_disk

        if self.options.fds_root.endswith ('/'):
            self.disk_config_file = self.options.fds_root + self.options.disk_config_file
        else:
            self.disk_config_file = self.options.fds_root + "/" + self.options.disk_config_file
    
        if DEFAULT_SHORT_PATH_AND_HINTS != self.options.disk_config_file:
            if DEFAULT_FDS_ROOT != self.options.fds_root:
                system_exit ('--fds-root (-f) and --map (-m) can not be used on the same command line')
            self.disk_config_file = self.options.disk_config_file


    def load_disk_config_file (self):

        input_file = open (self.disk_config_file, 'r')
    
        # reach each line from the hints file and create a disk
        for line in input_file:
            # items vector contains
            # [0] device path (/dev/sdm)
            # [1] bool, used by the OS
            # [2] bool, used for live meta data
            # [3] device type (ssd/hdd)
            # [4] define interface (sata/sas/na), doesn't matter for SSDs.
            # [5] size (GB)
    
            items = line.strip ('\r\n').split()
            if '#' == items[0][0]:                 # skip lines beginning with '#'
                continue

            disk = Disk (items[0], distutils.util.strtobool (items[1]), distutils.util.strtobool (items[2]), items[3], items[4], items[5])
            self.disk_list.append (disk)
    
        for disk in self.disk_list:
            disk.dump()

        '''
        # This feature is not supported at this time.
        if options.device:
            disk_list = [ x for x in disk_list if x.get_path() == options.device ]
            dbg_print ("=========== short list =========")
            for disk in disk_list:
                disk.dump()
        ''' 


    def disk_report (self):

       print "Your disks have been loaded from " + self.disk_config_file  + " as follow."

       for d in self.disk_list:
           d.print_disk()

       print "\nTo format or reformat disk(s), please use --format or --reset.  See --help for additional information"
       self.system_exit('')


    def verify_fresh_disks (self):
        ''' Make sure the disks are "new" to FDS or the --reset option must be used. '''

        fds_detected = False

        for disk in self.disk_list:
            if disk.check_for_fds():
                fds_detected = True;

        if fds_detected:
            self.system_exit ('Please use --reset to repartition and reformat all drives.')


    def calculate_capacities (self):
        ''' calculate the system capacity and index sizing '''

        index_capacity = 0
        total_capacity = 0

        for disk in self.disk_list:
            if disk.get_os_usage():
                continue
            disk_capacity = int (disk.get_capacity())
            if disk.get_index():
                index_capacity += disk_capacity
            total_capacity += disk_capacity
    
        if 1.0 * index_capacity / total_capacity > HIGH_INDEX_MODE_VALUE:            # force to floating point math
            self.dbg_print ("Using high index to storage capacity mode")
            usable_capacity = total_capacity
        else:
            usable_capacity = total_capacity - index_capacity
    
        usable_capacity_bytes = usable_capacity * 1000 * 1000 * 1000
    
        blob_count = usable_capacity_bytes / OBJECT_SIZE
    
        self.dm_index_MB = int (math.ceil (1.0 * blob_count * DM_INDEX_BYTES_PER_OBJECT / 1024 / 1024))
        self.sm_index_MB = int (math.ceil (1.0 * blob_count * SM_INDEX_BYTES_PER_OBJECT / 1024 / 1024))

        self.dbg_print ("Index capacity = " + str (index_capacity))
        self.dbg_print ("total_capacity= " + str (total_capacity))
        self.dbg_print ("usable_capacity_bytes = " + str (usable_capacity_bytes))
        self.dbg_print ("object_count = " + str (blob_count))
        self.dbg_print ("dm_index_MB = " + str (self.dm_index_MB))
        self.dbg_print ("sm_index_MB = " + str (self.sm_index_MB))
    
  
    def build_partition_lists (self):
        ''' Determine the needed partitions given the config file '''

        for disk in self.disk_list:
            if disk.get_os_usage():
                continue
            if disk.get_index():
                if len (self.dm_index_partition_list) < 1:
                    self.dm_index_partition_list.append (disk.get_path() + '2')
                    disk.set_dm_flag()
                else:
                    self.sm_index_partition_list.append (disk.get_path() + '2')
                    disk.set_sm_flag()
                self.data_partition_list.append (disk.get_path() + '3')
            else:
                self.data_partition_list.append (disk.get_path() + '2')

            # Build a list of partitions that may need to be unmounted
            self.umount_list = self.data_partition_list + self.dm_index_partition_list


    def partition_and_format_disks (self):
        ''' Partition and format each disk '''

        for disk in self.disk_list:
            if disk.get_os_usage():
                continue
            disk.partition (self.dm_index_MB, self.sm_index_MB / len (self.sm_index_partition_list))
            disk.format()


    def add_mount_point (self, uuid, mount_point):

        self.fstab.add_mount_point ('UUID=' + uuid + WHITE_SPACE + mount_point + WHITE_SPACE + PARITION_TYPE + WHITE_SPACE + MOUNT_OPTIONS + WHITE_SPACE + '0 2')


    def add_sm_mount_point (self):

        sm_uuid = None
    
        # See if there are enough SSD present to create a raid array for Data manager index storage
        if len (self.sm_index_partition_list) > 1:
            sm_uuid = self.raid_manager.create_index_raid (self.sm_index_partition_list)
        else:
            sm_uuid = self.get_uuid (self.sm_index_partition_list[0])
    
        # Add mount points to the fstab for Formation file systems
        self.add_mount_point (sm_uuid, SM_INDEX_MOUNT_POINT)


    def add_dm_mount_point (self):
        dm_uuid = self.disk_utils.get_uuid (self.dm_index_partition_list[0])
        self.add_mount_point (dm_uuid, DM_INDEX_MOUNT_POINT)


    def add_data_mount_points (self):
        hdd_count = 1
        ssd_count = 1
    
        for part in data_partition_list:
            part_uuid = get_uuid (part)
            disk_type = find_disk_type (disk_list, part)
    
            if Disk.DISK_TYPE_HDD == disk_type:
                mount_point = BASE_HDD_MOUNT_POINT + str (hdd_count)
                hdd_count += 1
            else:
                mount_point = BASE_SSD_MOUNT_POINT + str (ssd_count)
                ssd_count += 1
    
            new_fstab.add_mount_point (part_uuid, mount_point)


    def process (self):

        self.load_disk_config_file()

        # Harmless default action
        if not self.options.reset and not self.options.format:
            self.disk_report()

        if self.options.format and not self.options.reset:
            self.verify_fresh_disks()

        self.calculate_capacities()
        self.build_partition_lists()

        fstab_file = self.options.fstab
        self.fstab.read (fstab_file)

        self.raid_manager = RaidDevice()

        if not self.raid_manager.cleanup_raid_if_in_use (self.sm_index_partition_list, self.fstab, self.disk_utils):
            self.umount_list += self.sm_index_partition_list

        self.disk_utils.cleanup_mounted_file_systems (self.fstab, self.umount_list)

        self.partition_and_format_disks()
        self.add_sm_mount_point()
        self.add_dm_mount_point()
 
        # Replace the fstab
        if self.fstab.backup_if_altered (fstab_file):
            self.fstab.write (fstab_file)


def main():

    manager = DiskManager()
    manager.process_command_line()
    manager.process()


if __name__ == "__main__":

    main()
