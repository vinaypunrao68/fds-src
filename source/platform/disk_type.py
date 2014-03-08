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

    p_part = re.compile('([0-9]):(.+):(.+):(.+):([a-z]+):(.+):(.+)')
    fds_part_type = 'xfs'
    dsk_part_inv  = 'disk_part_inv'

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
            print 'WARNING: Nominal Medial Rotation Rate not found, default to ' + \
                  Disk.dsk_typ_hdd
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


    # Parse the disk information
    #
    def __init__(self, path):
        assert path != None
        assert re.match(Disk.dsk_prefix, path)

        self.dsk_path = path
        self.dsk_cap  = 0
        self.dsk_typ  = Disk.dsk_typ_inv
        self.dsk_part = Disk.dsk_part_inv

        self.__parse_with_hdparm(path)

        # Parse Disk parition type
        part_info = subprocess.Popen(['parted', '-m', path, 'print'],
                                     stdout=subprocess.PIPE).stdout

        for rec in part_info:
            ma = Disk.p_part.match(rec)
            if ma:
                # Partition
                dbg_print('Partition Found: ' + ma.group(5))
                assert self.dsk_part == Disk.dsk_part_inv or self.dsk_part == ''
                self.dsk_part = ma.group(5)

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

    def get_part(self):
        return self.dsk_part

    def create_part(self):
        return 0

    def makefs(self):
        return 0

    # print all the parsed fields
    #
    def print_disk(self):
        print '-------------------------------'
        print 'path     :\t' + self.get_path()
        print 'cap (GB) :\t' + str(self.get_capacity())
        print 'type     :\t' + self.get_type()
        print 'part     :\t' + self.get_part()

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

    # get all Disk objects in the system
    # 
    @staticmethod
    def sys_disks():
        dev_list  = []
        path_list = Disk.__get_sys_disks_path()
        for path in path_list:
            disk = Disk(path)
            dev_list.append(disk)
        return dev_list

# Turn on debugging
debug_on = False

def dbg_print(msg):
    if debug_on:
        print 'Debug: ' + msg

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Start FDS Processes...')
    parser.add_argument('--dev', help='dev path to get info')
    parser.add_argument('--format', help='create partition and format')
    parser.add_argument('--debug', help='turn on debugging')


    args = parser.parse_args()
    if args.dev:
        dev_path = args.dev
    else:
        dev_path = None
    if args.debug:
        debug_on = True

    if dev_path != None:
        disk = Disk(dev_path)
        disk.print_disk()
        sys.exit(0)

    dev_list = Disk.sys_disks()
    for disk in dev_list:
        disk.print_disk()
