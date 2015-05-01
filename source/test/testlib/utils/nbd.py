##############################################################################
# author: Philippe Ribeiro                                                   #
# email: philippe@formationds                                                #
# file: nbd.py                                                               #
##############################################################################
import apt
import argparse
import glob
import logging
import os
import subprocess
import sys
import shutil
import sh

nbd_path = os.path.abspath(os.path.join('..', '..'))
sys.path.append(nbd_path)

# import nbdadmin.py:
import cinder.nbdadm as nbdadm

class NBD(object):
    
    logging.basicConfig(level=logging.INFO,
                        format='%(asctime)s %(name)-12s %(levelname)-8s %(message)s',
                        datefmt='%m-%d %H:%M')
    log = logging.getLogger(__name__)
    
    def __init__(self, volume, mount_point, hostname=None, ipaddress=None):
        assert hostname or ipaddress
        # check if /dev/nbd* are present
        self.nbd_points = glob.glob('/dev/nbd*')
        # assert nbd mount points are present
        assert self.nbd_points != []
        # check if nbd-client is installed (package)
        cache = apt.Cache()
        if not cache['nbd-client'].is_installed:
            raise ImportError("Pacakge nbd-client not installed")
        # check if psutils is installed (package)
        # a hash between the filename and the md5 value
        self.files_table = {}
        self.host = ipaddress if ipaddress else hostname
        self.is_attached = False
        self.mount_point = mount_point
        self.is_mounted = False
        self.volume = volume
        self.lock_file = "/tmp/lock"
    
    # check if no device is left to be used
    # nbd attach [hostname / ip] [volume]
    # format the device
    # mkfs.ext4 [nbd_device]
    # mount [ndb_device] [mount_point]
    # copy data
    
    def check_status(self):
        self.log.info(self.is_mounted)
        self.log.info(self.host)
        self.log.info(self.nbd_points)

    def format_disk(self):
        # format the disk locally.
        pass

    def attach_nbd(self):
        try:
            self.log.info("Attaching volume [{}] to host[{}]".format(self.volume,
                                                                     self.host))
            args = ["attach", self.host, self.volume]
            parser = nbdadm.get_parser()
            result = parser.parse_args(args)
            with nbdadm.lock():
                self.is_attached = True
                return nbdadm.attach(result)
        except Exception, e:
            self.log.exception(e)
            sys.exit(2)

    def is_nbd_attached(self):
        # check if the volume is already being used
        return self.is_attached

    def detach_nbd(self):
        # detach nbd from the volume
        pass

    def check_data(self):
        # check if the previous data are there
        # by doing a md5
        current_files = os.listdir(self.mount_point)
        for current in current_files:
            if current in self.files_table:
                print "hello"

    def copy_data(self, files=[]):
        # copy the list of data files to the mount point
        # check if write date is READ_ONLY
        # check if the mount point is READ_ONLY
        try:
            if not self.is_attached:
                msg = "File system is not mounted"
                self.log.exception(msg)
                raise IOError(msg)
            for f in files:
                shutil.copy(f, self.mount_point)
                # calculate the md5 and store in the files table
                if f not in self.files_table:
                    self.files_table[f] = None
        except OSError as e:
            if e.errno == errno.EACCESs:
                self.log.exception("Mount point is not writable")
            else:
                self.log.exception(e)
    
    def mount_device(self, mount_point, nbd_device):
        # check if mount point exists
        # otherwise create one
        # mount the device
        if os.path.exists(mount_point):
            self.log.warning("Mount point {} exists.".format(mount_point))
            return
        # create the mount mount directory
        try:
            os.mkdir(self.mount_point)
            sh.mount(nbd_device, mount_point, "-t ext4")
            self.log.info("Device {} successfully mounted at {}".format(nbd_device,
                                                                       mount_point))
        except Exception, e:
            self.log.exception(e)
            self.log.info("Attempting to clean the {} mount point".format(mount_point))
            self.unmount_device(mount_point)
    
    def unmount_device(self, mount_point):
        # umount the device
        # remove mount point
        if os.path.exists(mount_point):
            self.log.info("Unmounting device {}".format(mount_point))
            sh.umount(mount_point)
            self.log.info("Cleaning up...")
            shutil.rmtree(mount_point)