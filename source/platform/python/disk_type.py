#!/usr/bin/env python
import sys
import os
import pdb
import optparse
import subprocess
import re

# Turn on debugging
debug_on = False

# Disk information parse from system
#
class Disk:
    # Static disk parsing stuff
    #
    p_dev   = re.compile('sd[a-z]$')
    dsk_prefix = '/dev/'
    dsk_part_hdd_data = 3
    dsk_part_ssd_data = 1

    p_type = re.compile('(.*Nominal Media Rotation Rate:\s+)(.+)')
    p_ssd   = re.compile('.*Solid State Device.*')
    dsk_typ_ssd = 'SSD'
    dsk_typ_hdd = 'HDD'
    dsk_typ_inv = 'disk_type_inv'

    p_info = re.compile('(' + dsk_prefix + '.+):(.+):(.+):(.+):(.+):(.+):(.+)')

    # We are using 1000*1000
    # device size with M = 1024*1024:     1907729 MBytes
    # device size with M = 1000*1000:     2000398 MBytes (2000 GB)
    p_size  = re.compile('(\s*device size with M = 1000\*1000:\s*)([0-9+].*)(MBytes)')
    dsk_ksize = 1000

    p_part = re.compile('([0-9]):(.+):(.+):(.+):([a-z]*):(.+):(.+)')
    fds_part_type = 'xfs'

    dsk_hdr_size = 32
    dsk_hdr_magic = '\xfe\xca\xbe\xba\xfe\xca\xbe\xba\xfe\xca\xbe\xba\xfe\xca\xbe\xba' \
                    '\xfe\xca\xbe\xba\xfe\xca\xbe\xba\xfe\xca\xbe\xba\xfe\xca\xbe\xba'
    dsk_hdr_clrmg = '\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00' \
                    '\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00'

## Public member functions
## -----------------------
##
    # Parse the disk information
    #
    def __init__(self, path, fake):
        assert path != None
        assert re.match(Disk.dsk_prefix, path)

        self.dsk_path = path
        self.dsk_cap  = 0
        self.dsk_typ  = Disk.dsk_typ_inv
        self.dsk_parts = { }
        self.dsk_formatted = 'Unknown'

        # parse disk information
        self.__parse_with_hdparm(path, fake)
        self.__parse_with_parted(path, fake)

        # check if disk is formatted
        self.__read_fds_hdr(fake)

    # test for dev is root device
    #
    def is_boot_dev(self):
        root_info = subprocess.Popen(['df', '/'], stdout=subprocess.PIPE).stdout
        for rec in root_info:
            ma = re.match(self.dsk_path, rec)
            if ma:
                dbg_print('Root Device Found: ' + ma.group(0))
                return True

        return False

    def is_mounted(self, df_output):
        for rec in df_output:
            ma = re.match(self.dsk_path, rec)
            if ma:
                dbg_print('Mounted device' + self.dsk_path)
                return True
        return False

    # drive type: HDD|SSD
    #
    def get_type(self):
        return self.dsk_typ

    # disk capacity in GB
    #
    def get_capacity(self):
        return self.dsk_cap

    # file system path of disk
    #
    def get_path(self):
        return self.dsk_path

    # list of partitions
    #
    def get_part(self):
        return self.dsk_parts

    # is fds formatted?
    def is_formatted(self):
        return self.dsk_formatted

    # print all the parsed fields
    #
    def print_disk(self):
        print '-------------------------------'
        print 'path     :\t' + self.get_path()
        print 'cap (GB) :\t' + str(self.get_capacity())
        print 'type     :\t' + self.get_type()
        parts = self.get_part()
        for k, v in parts.iteritems():
            print 'part     :\t' + k + ':' + v
        print 'formatted:\t' + str(self.is_formatted())

## ----------------------------------------------------------------
## destructive operations, handle with care
## make sure to check for is_boot_dev()

    # create partitions for HDD/SSD, write fds magic marker to drive
    #
    def create_parts(self):
        if self.is_boot_dev():
            return 1

        # Blindly remove any outstanding partitions.
        call_list = ['parted', '-s', self.dsk_path,
                     'rm', '1', ' ', 'rm', '2', ' ', 'rm', '3', ' ', 'rm', '4']
        print call_list
        res = subprocess.call(call_list, stdout = None, stderr = None)

        call_list = ['parted', '-s', self.dsk_path, 'mklabel', 'gpt']
        res = subprocess.call(call_list, stdout = None, stderr = None)
        if res != 0:
            print call_list
            return res

        if self.dsk_typ == Disk.dsk_typ_hdd:
            call_list = ['parted', '-s', '-a', 'opt', self.dsk_path,
                         '\"', 'mkpart', 'primary', 'xfs', '8MB', '8GB', '\"']
            res = subprocess.call(call_list, stdout = None, stderr = None)
            if res != 0:
                return res

            call_list = ['parted', '-s', '-a', 'none', self.dsk_path,
                         '\"', 'mkpart', 'primary', 'xfs', '8GB', '48GB', '\"']
            res = subprocess.call(call_list, stdout = None, stderr = None)
            if res != 0:
                return res
            # use shell=True for -1 argument
            call_list = "parted -s -a none " + self.dsk_path + \
                        " \"mkpart primary xfs 48GB -1\""
            res = subprocess.call(call_list, shell=True)
            if res != 0:
                return res
        elif self.dsk_typ == Disk.dsk_typ_ssd:
            call_list = "parted -s -a none " + self.dsk_path + \
                        " \"mkpart primary xfs 8MB -1\""
            res = subprocess.call(call_list, shell=True)
            if res != 0:
                return res
        else:
            assert not 'Invalid disk type found'
            return 2

        return res

    # delete partitions, wipe out fds magic marker
    #
    def del_parts(self):
        if self.is_boot_dev():
            return 1

        call_list = ['parted', '-s', self.dsk_path, 'mktable', 'gpt']
        print call_list
        res = subprocess.call(call_list)
        if res == 0:
            self.__write_fds_hdr(clear=True)
        return res

    # format using xfs, write magic header
    #
    def format(self):
        if self.is_boot_dev():
            return None

        res = {}
        res['dev_name'] = self.dsk_path
        if self.dsk_typ == Disk.dsk_typ_hdd:
            part_num = Disk.dsk_part_hdd_data
        elif self.dsk_typ == Disk.dsk_typ_ssd:
            part_num = Disk.dsk_part_ssd_data
        else:
            assert not 'Invalid disk type found'

        call_list = ['mkfs.xfs', self.dsk_path + str(part_num), '-f', '-q']
        res['dev_pipe'] = subprocess.Popen(call_list)
        return res

    def mount_fs(self, df_output, dev_no):
        if self.is_boot_dev():
            return None

        if self.is_mounted(df_output) == True:
            return None

        dsk_type = ''
        if self.dsk_typ == Disk.dsk_typ_hdd:
            dsk_type = 'hdd-%d' % dev_no
            part_num = Disk.dsk_part_hdd_data
        elif self.dsk_typ == Disk.dsk_typ_ssd:
            dsk_type = 'ssd-%d' % dev_no
            part_num = Disk.dsk_part_ssd_data
        else:
            assert not 'Invalid disk type found'

        dir_name  = '/fds/dev/' + dsk_type
        call_list = ['mkdir', '-p', dir_name]
        print call_list
        subprocess.call(call_list)

        call_list = ['mount', self.dsk_path + str(part_num), dir_name]
        print call_list
        subprocess.call(call_list)

## ----------------------------------------------------------------

    # get all Disk objects in the system, excluding boot device
    # 
    @staticmethod
    def sys_disks(fake):
        dev_list  = []
        path_list = Disk.__get_sys_disks_path(fake)
        for path in path_list:
            disk = Disk(path, fake)
            if not disk.is_boot_dev():
                dev_list.append(disk)
            else:
                print 'Skipping Root Device: ' + path
        return dev_list

## Private member functions
## ------------------------
##
    def __parse_with_hdparm(self, path, fake):
        # Parse for Disk type, capacity
        if fake == True:
            disk_info = self.__fake_hdparm(path)
        else:
            disk_info = subprocess.Popen(['hdparm', '-I', path],
                                         stdout=subprocess.PIPE).stdout
        found = 0
        total_rec = 2
        for rec in disk_info:
            # dbg_print(rec)
            mt = Disk.p_type.match(rec)
            if mt:
                dbg_print('HDP Type Found: ' + mt.group(1) + mt.group(2))
                is_ssd = Disk.p_ssd.match(mt.group(2))
                if is_ssd:
                    self.dsk_typ = Disk.dsk_typ_ssd
                else:
                    self.dsk_typ = Disk.dsk_typ_hdd
                found += 1

            ms = Disk.p_size.match(rec)
            if ms:
                dbg_print('HDP Capacity Found: ' + ms.group(1) + ms.group(2))
                found += 1
                # Convert to GB
                self.dsk_cap = int(ms.group(2)) / Disk.dsk_ksize

            if found == total_rec:
                break

        if self.dsk_typ == Disk.dsk_typ_inv:
            dbg_print('WARNING: Nominal Medial Rotation Rate not found, default to ' + \
                      Disk.dsk_typ_hdd)
            self.dsk_typ = Disk.dsk_typ_hdd

    def __parse_with_parted(self, path, fake):
        # Parse Disk parition type
        part_info = self.__get_disk_partition_type(path, fake)

        for rec in part_info:
            ma = Disk.p_part.match(rec)
            if ma:
                # Partition
                dbg_print('Partition Found: ' + ma.group(1) + ':' + ma.group(5))
                self.dsk_parts[ma.group(1)] = ma.group(5)

    def __fake_parted(self, path):
        res = ["BYT;\n",
                path + ":2199GB:scsi:512:512:gpt:ATA VBOX HARDDISK;\n",
                "1:17.4kB:100MB:100MB:xfs:primary:;\n",
                "2:100MB:32.0GB:31.9GB:xfs:primary:;\n",
                "3:32.0GB:2199GB:2167GB:xfs:primary:;\n"]
        return res
        
    def __fake_hdparm(self, path):
        res = ["\n" + path + ":\n\n",
            "ATA device, with non-removable media\n",
            "        Model Number:       VBOX HARDDISK\n",
            "        Serial Number:      VB6059e596-e3e22b0a\n",
            "        Firmware Revision:  1.0\n",
            "Standards:\n",
            "        Used: ATA/ATAPI-6 published, ANSI INCITS 361-2002\n",
            "        Supported: 6 5 4 \n",
            "Configuration:\n",
            "        Logical         max     current\n",
            "        cylinders       16383   16383\n",
            "        heads           16      16\n",
            "        sectors/track   63      63\n",
            "        --\n",
            "        CHS current addressable sectors:   16514064\n",
            "        LBA    user addressable sectors:  268435455\n",
            "        LBA48  user addressable sectors: 4294965248\n",
            "        Logical/Physical Sector size:           512 bytes\n",
            "        device size with M = 1024*1024:     2097151 MBytes\n",
            "        device size with M = 1000*1000:     2199022 MBytes (2199 GB)\n",
            "        cache/buffer size  = 256 KBytes (type=DualPortCache)\n",
            "Capabilities:\n",
            "      LBA, IORDY(cannot be disabled)\n",
            "      Queue depth: 32\n",
            "      Standby timer values: spec'd by Vendor, no device specific minimum\n",
            "      R/W multiple sector transfer: Max = 128 Current = 128\n",
            "      DMA: mdma0 mdma1 mdma2 udma0 udma1 udma2 udma3 udma4 udma5 *udma6 \n",
            "            Cycle time: min=120ns recommended=120ns\n",
            "       PIO: pio0 pio1 pio2 pio3 pio4 \n",
            "            Cycle time: no flow control=120ns  IORDY flow control=120ns\n",
            "Commands/features:\n",
            "       Enabled Supported:\n",
            "          *    Power Management feature set\n",
            "          *    Write cache\n",
            "          *    Look-ahead\n",
            "          *    48-bit Address feature set\n",
            "          *    Mandatory FLUSH_CACHE\n",
            "          *    FLUSH_CACHE_EXT\n",
            "          *    Gen2 signaling speed (3.0Gb/s)\n",
            "          *    Native Command Queueing (NCQ)\n",
            "Checksum: correct\n"]
        return res

    def __get_disk_partition_type(self, path, fake):
        if fake == True:
            out =self. __fake_parted(path)
        else:
            out = subprocess.Popen(['parted', '-m', path, 'print'],
                                         stdout=subprocess.PIPE).stdout
        return out

    # Cannot parse disk not partitioned with gpart
    #
    def __parse_with_parted_type_cap(self, path):
        # Parse Disk parition type
        part_info = subprocess.Popen(['parted', '-m', path, 'print'],
                                     stdout=subprocess.PIPE).stdout

        for rec in part_info:
            ma = Disk.p_info.match(rec)
            if ma:
                # HDD/SSD
                dbg_print('Type Found: ' + ma.group(7))
                is_ssd = Disk.p_ssd.match(ma.group(7))
                if is_ssd:
                    self.dsk_typ = Disk.dsk_typ_ssd
                else:
                    self.dsk_typ = Disk.dsk_typ_hdd

                # Capacity
                self.dsk_cap = re.match('[0-9]+', ma.group(2)).group(0)
                self.dsk_cap = int(self.dsk_cap)
                dbg_print('Capacity Found: ' + ma.group(2))

    # read header, set formatted field if header is found
    def __read_fds_hdr(self, fake):
        if fake == True:
            self.dsk_formatted = True
            return

        fp = open(self.dsk_path, 'rb')
        hdr_info = fp.read(Disk.dsk_hdr_size)
        fp.close()
        if hdr_info == Disk.dsk_hdr_magic:
            dbg_print('Magic found: ' + hdr_info)
            self.dsk_formatted = True
        else:
            dbg_print('Magic not found' + hdr_info)
            self.dsk_formatted = False

    # write fds header
    def __write_fds_hdr(self, clear=False):
        fp = open(self.dsk_path, 'w')

        if debug_on:
            print 'Writing FDS Hdr to disk:' + self.dsk_path
        if clear == False:
            hdr_val = Disk.dsk_hdr_magic
        else:
            hdr_val = Disk.dsk_hdr_clrmg

        hdr_s = ':'.join(x.encode('hex') for x in hdr_val)
        if debug_on:
            print hdr_s
        fp.write(hdr_val)

        fp.close()
        self.__read_fds_hdr(False)

    @staticmethod
    def __get_fake_sys_disk_path():
        res = ['/dev/fake1', '/dev/fake2', '/dev/fake3', '/dev/fake4',
               '/dev/fake5', '/dev/fake6', '/dev/fake7', '/dev/fake8',
               '/dev/fake9', '/dev/fake10', '/dev/fake11', '/dev/fake12']
        return res

    @staticmethod
    def __get_sys_disks_path(fake):
        if fake == True:
            return Disk.__get_fake_sys_disk_path()

        path_info = subprocess.Popen(['ls', Disk.dsk_prefix],
                                     stdout=subprocess.PIPE).stdout
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
        print 'Debug: ' + msg

def disk_helper(disk, fmt, clear_fmt):
    assert not (fmt == True and clear_fmt == True)
    if debug_on:
        disk.print_disk()

    if fmt == True:
        if disk.create_parts() == 0:
            return disk.format()
    return None

if __name__ == "__main__":
    parser = optparse.OptionParser("usage: %prog [options]")
    parser.add_option('-d', '--dev', dest = 'device',
                      help = 'Path to get dev info (e.g. /dev/sdb)')
    parser.add_option('-f', '--format', dest = 'format', action = 'store_true',
                      help = 'Parition and format the device')
    parser.add_option('-m', '--mount', dest = 'mount', action = 'store_true',
                      help = 'Mount filesystem to the partition data')
    parser.add_option('-F', '--fake', dest = 'fake', action = 'store_true',
                      help = 'Create fake device for testing')
    parser.add_option('-D', '--debug', dest = 'debug', action = 'store_true',
                      help = 'Turn on debugging')

    (options, args) = parser.parse_args()
    debug_on = options.debug

    if options.format:
        shells = []
        if options.device:
            disk = Disk(options.device, options.fake)
            cb = disk_helper(disk, options.format, False)
            shells.append(cb)
        else:
            dev_list = Disk.sys_disks(options.fake)
            for disk in dev_list:
                cb = disk_helper(disk, options.format, False)
                shells.append(cb)

        for cb in shells:
            if cb is not None:
                print "Waiting to format device ", cb['dev_name']
                cb['dev_pipe'].wait()

    if options.mount:
        df_output = subprocess.Popen(['df', '/'], stdout=subprocess.PIPE).stdout
        if options.device:
            disk = Disk(options.device, options.fake)
            disk.mount_fs(df_output)
        else:
            dev_cnt  = 0
            dev_list = Disk.sys_disks(options.fake)
            for disk in dev_list:
                disk.mount_fs(df_output, dev_cnt)
                dev_cnt = dev_cnt + 1
