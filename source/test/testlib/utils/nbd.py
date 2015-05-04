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
import time
import timeout_decorator

nbd_path = os.path.abspath(os.path.join('..', '..'))
sys.path.append(nbd_path)

# import nbdadmin.py:
import cinder.nbdadm as nbdadm

class NBD(object):

    logging.basicConfig(level=logging.INFO,
                        format='%(asctime)s %(name)-12s %(levelname)-8s %(message)s',
                        datefmt='%m-%d %H:%M')
    log = logging.getLogger(__name__)

    def __init__(self, hostname, mount_point):
        assert hostname is not None
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
        self.devices = {}
        for nbd in self.nbd_points:
            self.devices[nbd] = None

        self.host = hostname
        self.is_attached = False
        self.mount_point = mount_point
        self.is_mounted = False

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

    def format_disk(self, device):
        # format the disk locally.
        status = False
        self.log.info("Formatting device: {}".format(device))
        try:
            res = sh.mkfs(device)
            self.log.info("Format result: {}".format(res))
            status = True
        except Exception, e:
            self.log.excetion(e)
            status = False
        finally:
            return True

    # timeout if attach is taking more than 30 seconds
    @timeout_decorator.timeout(30)
    def attach(self, volume):
        try:
            self.log.info("Attaching volume [{}] in host[{}]".format(volume,
                                                                     self.host))
            args = ["attach", self.host, volume]
            parser = nbdadm.get_parser()
            result = parser.parse_args(args)
            with nbdadm.lock():
                (device, code) = nbdadm.attach(result)
                if device is not None:
                    assert device in self.nbd_points
                    self.devices[device] = volume
                    self.log.info("Attached to device: {}".format(device))
                    return (device, code)
                else:
                    raise ValueError("nbd client failed with error code {}".format(code))
        except Exception, e:
            self.log.exception(e)
            sys.exit(2)

    def is_nbd_attached(self):
        # check if the volume is already being used
        return self.is_attached

    @timeout_decorator.timeout(30)
    def detach(self, volume):
        # detach nbd from the volume
        try:
            self.log.info("Detaching volume [{}] in host[{}]".format(volume,
                                                                     self.host))
            args = ["detach", self.host, volume]
            parser = nbdadm.get_parser()
            result = parser.parse_args(args)
            with nbdadm.lock():
                    retcode = nbdadm.detach(result)
                    if retcode == 0:
                        for device, v in self.devices.iteritems():
                            if v == volume:
                                self.devices[device] = None
                    return retcode
        except Exception, e:
            self.log.exception(e)
            sys.exit(2)

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

    def mount(self, mount_point, device):
        # check if mount point exists
        # otherwise create one
        # mount the device
        if os.path.exists(mount_point):
            self.log.warning("Mount point {} exists.".format(mount_point))
            return -1
        # create the mount mount directory
        try:
            os.mkdir(self.mount_point)
            sh.mount(device, mount_point)
            self.log.info("Device {} successfully mounted at {}".format(device,
                                                                       mount_point))
            return 0
        except Exception, e:
            self.log.exception(e)
            self.log.info("Attempting to clean the {} mount point".format(mount_point))
            self.unmount(mount_point)
            return -1

    def unmount(self, mount_point):
        # umount the device
        # remove mount point
        if os.path.exists(mount_point):
            self.log.info("Unmounting device {}".format(mount_point))
            sh.umount(mount_point)
            self.log.info("Cleaning up...")
            shutil.rmtree(mount_point)
