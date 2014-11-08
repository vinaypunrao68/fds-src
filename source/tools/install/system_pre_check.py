#!/usr/bin/env python

''' 
system_pre_check.py -- Script to do pre-install verification of nodes

Usage within a script:

# Create a new InstallConfig instance
# Parameters: min_mem_in_GB, min_#_disks, min_disk_size_in_GB, wipe?
config = InstallConfig('MyUser', 'MyPassword, '10.10.10.1', 22, 2, 6, 1000, False)

# Verify all nodes
config.verify()

'''

import os
import sys
import re
import argparse
import subprocess
import pdb

# TODO: Remove path information when script is repackaged
sys.path.append('../../platform/python/')
import disk_type


# Error messages
SysPrecheckErrors = (
    "", # First must be empty otherwise errors will be off by one
    "Not enough available memory.",
    "Not enough available disk drives.",
    "Drive capacity not large enough.",
    "System unable to connect to internet.",
    "Incompatable OS version found."
)

class DefaultConfig:
    ''' Default configuration options. This should be the only location that
    these values need to be changed.
    '''
    min_mem = 32 # Mem size in GB
    num_disks = 6 # 6 disks
    disk_size = 490 # in GB, 11/08/2014, disk_part.py is reporting 499 GB on certain supermicro units.
    net_connect = False
    os_version = "Ubuntu 14.04"
    wipe = False

class InstallConfig:
    def __init__(self, min_mem=DefaultConfig.min_mem,
                 num_disks=DefaultConfig.num_disks,
                 disk_size=DefaultConfig.disk_size,
                 wipe=DefaultConfig.wipe, 
                 os_version=DefaultConfig.os_version,
                 net_connect=DefaultConfig.net_connect):
        
        ''' 
        Constructor for creating InstallConfigs in scripts by passing
        values in.
        '''
        # Wipe drives?
        self._wipe = wipe
        # Minimum memory
        self._min_mem = min_mem
        # Min number of disks
        self._num_disks = num_disks
        # Min disk size 
        self._disk_size = disk_size
        # OS version
        self._os_version = os_version
        # Net connect
        self._net_connect = net_connect

    @classmethod
    def from_cli_args(self, args):
        ''' 
        Method for creating InstallConfig using arguments collected using argparse from the command line.
        '''
        assert args.__class__ == argparse.Namespace, "Called from_cli_args with incorrect parameter type."

        return InstallConfig(args.min_mem, args.num_disks,
                             args.disk_size, args.wipe, 
                             args.os_version, args.net_connect)
    @property
    def wipe(self):
        return self._wipe

    @property
    def min_mem(self):
        return self._min_mem

    @property
    def min_disks(self):
        return self._num_disks

    @property
    def disk_size(self):
        return self._disk_size

    @property
    def os_version(self):
        return self._os_version

    @property
    def net_connect(self):
        return self._net_connect

    def __str__(self):
        return ("\nMIN_MEM = " +  str(self.min_mem) + 
                "\nMIN_DISKS = " + str(self.min_disks) +
                "\nDISK_SIZE = " + str(self.disk_size) + 
                "\nWIPE? = " + str(self.wipe))

    def verify(self):
        ''' 
        Performs node verification for this node.
        Returns:
          0 on success, ERROR_CODE otherwise.
        '''
        node = FDS_Node()
        res = node.verify(self)
        if res == 0:
            print "Node PASSED!"
        else:
            print "Node FAILED!", SysPrecheckErrors[res]
            return res

        return 0
                

class FDS_Node:
    def __init__(self):
        ''' Construct a node '''
        
        self.mem_size = None
        self.num_disks = None
        # Each disk in the list will be a disk_type.Disk and will keep
        # its own size information
        self.disks = [] 
        self.net_connect = False
        self.os_version = None
        # Populate node information
        self.populate_node_info()

    def __str__(self):
        return ("\nTOTAL_MEM = " +  str(self.mem_size) + 
                "\nNUM_DISKS = " + str(self.num_disks) + 
                "\nDISK_SIZE = " + str(self.disk_size))



    def populate_node_info(self):
        '''
        Grabs all of the relevant information.
        '''
        # Get memory information
        self.__get_mem_info()
        self.__get_disk_info()
        self.__get_os_version()
        self.__get_net_connect()

    def __get_mem_info(self):
        '''
        Cats /proc/info on a node and collects memory information for verification.
        Sets self.mem_size to value found (in GB).
        Returns:
          None
        '''
        
        print "Getting system memory information..."

        print "Calling cat /proc/meminfo ..."
        # Get the mem size        
        mem_info = subprocess.Popen(['cat', '/proc/meminfo'], 
                                    stdout=subprocess.PIPE).stdout

        print "Parsing output to find memory size..."
        # Find the MemTotal
        for line in mem_info:
            match = re.match('MemTotal:\s*(\d*)\s*(\w{2,3})', line)
            mem_size = 0
            mem_size_suffix = ""
            if match is not None:
                mem_size = int(match.group(1))
                mem_size_suffix = match.group(2)
            
                # Make mem_size in GB
                if mem_size > 0 and mem_size_suffix == "kB":
                    self.mem_size = mem_size / 1000 / 1000
                    
                return

    def __get_disk_info(self):
        '''
        Calls disk_type.Disk.sys_disks() to collect disk and partition
        information, and sets self.num_disks and self.disk_size
        members.

        Returns:
          None

        '''
        
        print "Getting system disk information..."

        print "Calling df on / ..."
        df_output    = subprocess.Popen(['df', '/'], stdout=subprocess.PIPE).stdout
        print "Calling mount ..."
        mount_output = subprocess.Popen(['mount'], stdout=subprocess.PIPE).stdout
        
        print "Processing output to gather disk information..."
        self.disks = disk_type.Disk.sys_disks(False, df_output, mount_output)
        
        self.num_disks = len(self.disks)


    def __get_os_version(self):
        '''
        Calls lsb_release -a and parses the output to find OS version.
        Sets self.os_version with the result.

        Returns:
          None
        '''

        print "Getting OS version information..."
        print "Calling lsb_release -a..."

        os_info = subprocess.Popen(['lsb_release', '-a'],
                                   stdout=subprocess.PIPE).stdout
        print "lsb_release -a returned, finding OS version..."

        for line in os_info:
            match = re.match('Description:\s*(.*)', line)
            if match is not None:
                self.os_version = match.group(1)

                return

    def __get_net_connect(self):
        '''
        Pings an external website to determine if the node has outside
        connectivity. Sets self.net_connect to True if node has internet 
        access, False otherwise.
        
        Returns:
          None
        '''

        print "Checking internet connection..."
        FNULL = open(os.devnull, 'w')
        print "Sending ping request to www.google.com..."
        net_info = subprocess.call(['ping', '-c 1', 'www.google.com'], stdout=FNULL)
        FNULL.close()
          
        if net_info == 0:
            self.net_connect = True
        else:
            self.net_connect = False

        
        

    def verify(self, config):

        ''' 
        Verify that this node meets the criteria specified in config.
        Params:
          config - InstallConfig containing the specified parameter values to verify
        
        Returns:
          0 on success, 1 otherwise
        '''

        # Do memory verification
        # If we're under on memory, error out
        if self.mem_size < config.min_mem:
            return 1
        
        # Verify number of disks
        if self.num_disks < config.min_disks:
            return 2

        # Verify disk size
        for disk in self.disks:
            # Verify that hard drive (non-SSD) meets the minimum size
            if (disk.get_type() == disk_type.Disk.dsk_typ_hdd and 
                disk.get_capacity() < config.disk_size):
                return 3
                
        # Only return an error on no net when specified otherwise via cli
        if config.net_connect and self.net_connect :
            return 4

        if self.os_version != config.os_version:
            return 5

        # If we've passed everything return 0
        return 0


if __name__ == "__main__":
    # Do the setup with provided arguments
    # Parse command line args
    arg_parser = argparse.ArgumentParser()

    arg_parser.add_argument('--min_mem', type=int, default=DefaultConfig.min_mem)
    arg_parser.add_argument('--num_disks', type=int, default=DefaultConfig.num_disks)
    arg_parser.add_argument('--disk_size', type=int, default=DefaultConfig.disk_size)
    arg_parser.add_argument('--wipe', action='store_const', const=True, default=DefaultConfig.wipe)
    arg_parser.add_argument('--os_version', default=DefaultConfig.os_version)
    arg_parser.add_argument('--net_connect', type=bool, default=DefaultConfig.net_connect)
    
    install_c = InstallConfig.from_cli_args(arg_parser.parse_args())
    sys.exit(install_c.verify())
