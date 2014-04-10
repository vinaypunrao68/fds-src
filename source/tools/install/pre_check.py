''' 
pre_check.py -- Script to do pre-install verification of nodes

Error codes:
 1 - Not enough system memory
 2 - Too few disks
 3 - Drive size too small

Usage within a script:

# Create a new InstallConfig instance
# Parameters: username, password, [ip addresses], port, min_mem_in_GB, min_#_disks, min_disk_size_in_GB, wipe?
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


class DefaultConfig:
    ''' Default configuration options. This should be the only location that
    these values need to be changed.
    '''
    min_mem = 2 # Mem size in GB
    num_disks = 6 # 6 disks
    disk_size = 1000 #2TB in GB
    wipe = False

class InstallConfig:
    def __init__(self, min_mem=DefaultConfig.min_mem,
                 num_disks=DefaultConfig.num_disks,
                 disk_size=DefaultConfig.disk_size,
                 wipe=DefaultConfig.wipe):
        
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

    @classmethod
    def from_cli_args(self, args):
        ''' 
        Method for creating InstallConfig using arguments collected using argparse from the command line.
        '''
        assert args.__class__ == argparse.Namespace, "Called from_cli_args with incorrect parameter type."

        return InstallConfig(args.min_mem, args.num_disks,
                             args.disk_size, args.wipe)
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
            print "Node FAILED! with error code:", res
            return res

        return 0
                

class FDS_Node:
    def __init__(self):
        ''' Construct a node '''
        
        self.mem_size = None
        self.num_disks = None
        # Each disk will keep its own size information
        self.disks = [] 
        
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
        

    def __get_mem_info(self):
        '''
        Cats /proc/info on a node and collects memory information for verification.
        Sets self.mem_size to value found (in GB).
        Returns:
          None
        '''

        # Get the mem size        
        mem_info = subprocess.Popen(['cat', '/proc/meminfo'], 
                                    stdout=subprocess.PIPE).stdout

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
                    self.mem_size = mem_size / 1024 / 1024
                    
                return

    def __get_disk_info(self):
        '''
        Calls disk_type.Disk.sys_disks() to collect disk and partition
        information, and sets self.num_disks and self.disk_size
        members.

        Returns:
          None

        '''

        self.disks = disk_type.Disk.sys_disks(False)
        self.num_disks = len(self.disks)


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
            # Verify that each meets the minimum size
            if disk.get_capacity() < config.disk_size:
                return 3
                
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

    install_c = InstallConfig.from_cli_args(arg_parser.parse_args())
    print install_c
    install_c.verify()
