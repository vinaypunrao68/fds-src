##############################################################################
# authors: Philippe Ribeiro                                                  #
#          Luke Guildner                                                     #
# email: philippe@formationds                                                #
# file: nbd.py                                                               #
##############################################################################

import os
import sys
import sh
#import timeout_decorator

nbd_path = os.path.abspath(os.path.join('..', '..', ''))
sys.path.append(nbd_path)

print sys.path

# import nbdadmin.py:
from cinder.nbdadm import nbdlib
from cinder.nbdadm import nbd_user_error
from cinder.nbdadm import nbd_client_error
from cinder.nbdadm import nbd_modprobe_error
from cinder.nbdadm import nbd_volume_error
from cinder.nbdadm import nbd_host_error

class nbd_mount_error(Exception):
    def __init__(self, value):
        self.value = value
    def __str__(self):
        return repr(self.value)

class nbd_unmount_error(Exception):
    def __init__(self, value):
        self.value = value
    def __str__(self):
        return repr(self.value)

class nbd_format_error(Exception):
    def __init__(self, value):
        self.value = value
    def __str__(self):
        return repr(self.value)

class NBD(nbdlib):

    def __init__(self, hostname):
        if hostname is None:
            raise ValueError("hostname must be set")
        self.get_device_paths()
        self.host = hostname
        super(NBD, self).__init__()

    def attach(self, volume):
        return super(NBD, self).attach(self.host, None, volume, True)

    def format_dev(self, device, fs_type):
        # build args to mkfs
        arg_list = []
        if fs_type is not None:
            arg_list.append('-t ')
            arg_list.append(fs_type)
        arg_list.append(device)
        # call mkfs
        response = sh.mkfs(arg_list)
        if response.exit_code is not 0:
            raise nbd_format_error("unknown error formatting device: {}".format(device))

    def mount_device(self, device, path):
        if device is None:
            raise ValueError("device must be set")
        if not os.path.exists(path):
            raise ValueError("mount path must exist")
        result = sh.mount(device, path)
        if result.exit_code is not 0:
            raise nbd_mount_error("unknown error mounting path: {}".format(path))

    def mount_volume(self, volume, path, format_volume=False, fs_type=None):
        if not os.path.exists(path):
            raise ValueError("mount path must exist")
        dev = super(NBD, self).attach(self.host, None, volume, True, False)
        if format_volume:
            self.format_dev(dev, fs_type)
        self.mount_device(dev, path)
        return dev

    #@timeout_decorator.timeout(30)
    def unmount(self, path):
        if os.path.exists(path): 
            if os.path.ismount(path):
                result = sh.umount(path)
                if result.exit_code is not 0:
                    raise nbd_unmount_error("unknown error unmounting path: {}".format(path))
