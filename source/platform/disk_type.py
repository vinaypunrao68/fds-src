#!/usr/bin/env python
import sys
import os
import pdb
import argparse
import subprocess
import re

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
    def __init__(self, path):
        assert path != None
        assert re.match(Disk.dsk_prefix, path)

        self.dsk_path = path
        self.dsk_cap  = 0
        self.dsk_typ  = Disk.dsk_typ_inv
        self.dsk_parts = { }
        self.dsk_formatted = 'Unknown'

        self.__parse_with_hdparm(path)
        # Parse Disk parition type
        part_info = subprocess.Popen(['parted', '-m', path, 'print'],
                                     stdout=subprocess.PIPE).stdout

        for rec in part_info:
            ma = Disk.p_part.match(rec)
            if ma:
                # Partition
                dbg_print('Partition Found: ' + ma.group(1) + ':' + ma.group(5))
                self.dsk_parts[ma.group(1)] = ma.group(5)

        # check if disk is formatted
        self.__read_fds_hdr()

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

        call_list = ['parted', '-s', self.dsk_path, 'mklabel', 'gpt']
        print call_list
        res = subprocess.call(call_list)
        if res != 0:
            return res

        if self.dsk_typ == Disk.dsk_typ_hdd:

            call_list = ['parted', '-s', '-a', 'none', self.dsk_path,
                         '\"', 'mkpart', 'primary', 'xfs', '0', '100MB', '\"']
            print call_list
            res = subprocess.call(call_list)
            if res != 0:
                return res

            call_list = ['parted', '-s', '-a', 'none', self.dsk_path,
                         '\"', 'mkpart', 'primary', 'xfs', '100MB', '32GB', '\"']
            print call_list
            res = subprocess.call(call_list)
            if res != 0:
                return res
            # use shell=True for -1 argument
            call_list = "parted -s -a none " + self.dsk_path + \
                        " \"mkpart primary xfs 32GB -1\""
            print call_list
            res = subprocess.call(call_list, shell=True)
            if res != 0:
                return res
        elif self.dsk_typ == Disk.dsk_typ_ssd:
            call_list = "parted -s -a none " + self.dsk_path + \
                        " \"mkpart primary xfs 0 -1\""
            print call_list
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
            return 1

        res = 0
        if self.dsk_typ == Disk.dsk_typ_hdd:
            part_num = Disk.dsk_part_hdd_data
        elif self.dsk_typ == Disk.dsk_typ_ssd:
            part_num = Disk.dsk_part_ssd_data
        else:
            assert not 'Invalid disk type found'

        call_list = ['mkfs.xfs', self.dsk_path + str(part_num), '-f', '-q']
        print call_list
        res = subprocess.call(call_list)
        if res == 0:
            self.__write_fds_hdr()
        return res
## ----------------------------------------------------------------

    # get all Disk objects in the system, excluding boot device
    # 
    @staticmethod
    def sys_disks():
        dev_list  = []
        path_list = Disk.__get_sys_disks_path()
        for path in path_list:
            disk = Disk(path)
            if not disk.is_boot_dev():
                dev_list.append(disk)
            else:
                print 'Skipping Root Device: ' + path
        return dev_list

## Private member functions
## ------------------------
##
    def __parse_with_hdparm(self, path):
        # Parse for Disk type, capacity
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

    # Cannot parse disk not partitioned with gpart
    #
    def __parse_with_parted(self, path):
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
    def __read_fds_hdr(self):
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

        print 'Writing FDS Hdr to disk:' + self.dsk_path
        if clear == False:
            hdr_val = Disk.dsk_hdr_magic
        else:
            hdr_val = Disk.dsk_hdr_clrmg

        hdr_s = ':'.join(x.encode('hex') for x in hdr_val)
        print hdr_s
        fp.write(hdr_val)

        fp.close()
        self.__read_fds_hdr()

    @staticmethod
    def __get_sys_disks_path():
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

# Turn on debugging
debug_on = False

def dbg_print(msg):
    if debug_on:
        print 'Debug: ' + msg

def disk_helper(disk, fmt, clear_fmt):
    assert not (fmt == True and clear_fmt == True)
    disk.print_disk()
    res = 0

    if fmt == True:
        if not disk.is_formatted():
            res = disk.create_parts()
            if res == 0:
                res = disk.format()
        else:
            print 'Disk is already formatted: ' + disk.get_path()
    elif clear_fmt == True:
        res = disk.del_parts()

    return res

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Disk Infor and Format.')
    parser.add_argument('--dev', help='dev path to get info')
    parser.add_argument('--format', help='create partition and format')
    parser.add_argument('--clear_fmt', help='clear formatted partition')
    parser.add_argument('--debug', help='turn on debugging')


    args = parser.parse_args()
    if args.dev:
        dev_path = args.dev
    else:
        dev_path = None
    if args.debug:
        debug_on = True

    if args.format == 'true':
        fmt = True
    else:
        fmt = False

    if args.clear_fmt == 'true':
        clear_fmt = True
    else:
        clear_fmt = False

    if fmt and clear_fmt:
        print 'Cannot use format and clear_fmt at the same time'
        sys.exit(1)

    if dev_path != None:
        disk = Disk(dev_path)
        disk_helper(disk, fmt, clear_fmt)
        sys.exit(0)

    dev_list = Disk.sys_disks()
    for disk in dev_list:
        disk_helper(disk, fmt, clear_fmt)
