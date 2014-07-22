#!/usr/bin/python
from __future__ import print_function
import shlex
import sys
import os
import subprocess
import re
import traceback

class AppendOperations(object):
    def __init__(self, out):
        self.out = out
        self.total_errs = 0
        self.total_missing_bins = 0

    def print_description(self, description):
        print('-' * (len(description) + 6), file=self.out)
        print('|  ' + description + '  |', file=self.out)
        print('-' * (len(description) + 6), file=self.out)
        print('', file=self.out)

    def print_failed(self, description):
        print('!' * (len(description) + 6), file=self.out)
        print('|  ' + description + '  |', file=self.out)
        print('!' * (len(description) + 6), file=self.out)
        print('', file=self.out)

    def bin_on_path(self, cmd):
        bin = cmd.split()
        ret = subprocess.call(['which', bin[0]],
                              stdout=subprocess.PIPE,
                              stderr=subprocess.PIPE)
        if ret == 0:
            return True
        return False

    def append_output(self, cmd, description=None):
        if not self.bin_on_path(cmd):
            self.total_missing_bins += 1
            return
        if description is None:
            description = "$ " + cmd
        try:
            fd = subprocess.Popen(
                cmd,
                stdout=subprocess.PIPE,
                shell=True)
            retcode = fd.wait()
            output_data = fd.stdout
            self.print_description(description)
            for line in output_data:
                print(line, file=self.out)
            print('', file=self.out)
            print("return code: ", retcode, "\n")
            if retcode != 0:
                self.total_errs += 1
        except Exception as e:
            self.print_failed(description)
            traceback.print_exc(self.out)
            print('', file=self.out)

    def append_file(self, filename, description=None, config=False):
        if description is None:
            description = filename

        try:
            with open(filename) as lines:
                self.print_description(description)
                for line in lines:
                    if not config or not re.match("^\s*(#.*)?", line) is None:
                        print(line.strip(), file=self.out)
        except Exception as e:
            self.print_failed(description)
            traceback.print_exc(self.out)
            print('', file=self.out)
    def print_summary(self):
        print("----------------------------------------------------------------")
        print("Total Errors: ", self.total_errs)
        print("Total Missing Binaries: ", self.total_missing_bins)
        print("----------------------------------------------------------------")
        

def main():
    ops = AppendOperations(sys.stdout)

    # General system configurations
    ops.append_output("hostname")
    ops.append_output("uname -a")
    ops.append_output("ip addr")
    ops.append_output("ip route")
    ops.append_output("ping www.google.com -c 10", description="Check Internet connection")
    #ops.append_output("lshw")
    ops.append_output("cat /proc/cpuinfo")
    ops.append_output("cat /proc/meminfo")
    ops.append_output("pvdisplay")
    ops.append_output("vgdisplay")
    ops.append_output("lvdisplay")

    # Openstack credentials

    # Openstack specific configurations
    # list all openstack services
    # identity - keystone
    ops.append_output("keystone --version")
    ops.append_output("keystone ec2-credentials-list")
    ops.append_output("keystone endpoint-list")
    ops.append_output("keystone role-list")
    ops.append_output("keystone service-list")
    ops.append_output("keystone tenant-list")
    ops.append_output("keystone user-list")
    ops.append_output("keystone user-role-list")

    # image - glance
    ops.append_output("glance --version")
    ops.append_output("glance image-list")

    # nova-mangage
    ops.append_output("nova-manage --version")
    ops.append_output("nova-manage version")
    ops.append_output("nova-manage host list")
    ops.append_output("nova-manage service list")
    ops.append_output("nova-manage fixed list")
    ops.append_output("nova-manage user list")
    ops.append_output("nova-manage project list")
    ops.append_output("nova-manage floating list")
    ops.append_output("nova-manage flavor list")
    ops.append_output("nova-manage instance_type list")
    ops.append_output("nova-manage vm list")
    ops.append_output("nova-manage agent list")
    ops.append_output("nova-manage network list")

    # compute - nova
    ops.append_output("nova --version")
    ops.append_output("nova agent-list")
    ops.append_output("nova availability-zone-list")
    ops.append_output("nova cloudpipe-list")
    ops.append_output("nova flavor-list")
    ops.append_output("nova floating-ip-bulk-list")
    ops.append_output("nova floating-ip-list")
    ops.append_output("nova floating-ip-pool-list")
    ops.append_output("nova host-list")
    ops.append_output("nova hypervisor-list")
    ops.append_output("nova image-list")
    ops.append_output("nova keypair-list")
    ops.append_output("nova list")
    ops.append_output("nova network-list")
    ops.append_output("nova secgroup-list")
    ops.append_output("nova service-list")
    ops.append_output("nova usage-list")
    ops.append_output("nova volume-list")
    ops.append_output("nova volume-snapshot-list")
    ops.append_output("nova volume-type-list")
    ops.append_output("nova list-extensions")
    ops.append_output("nova net-list")

    # dashboard - horizon

    # block - cinder
    ops.append_output("cinder --version")
    ops.append_output("cinder availability-zone-list")
    ops.append_output("cinder backup-list")
    ops.append_output("cinder encryption-type-list")
    ops.append_output("cinder extra-specs-list")
    ops.append_output("cinder list")
    ops.append_output("cinder qos-list")
    ops.append_output("cinder service-list")
    ops.append_output("cinder snapshot-list")
    ops.append_output("cinder transfer-list")
    ops.append_output("cinder type-list")
    ops.append_output("cinder list-extensions")

    # object storage - swift (TBD)
    # Network (TBD)
    # Orchestration (TBD)
    # Telemetry (TBD)

    ops.print_summary()

    return 0

if __name__ == '__main__':
    sys.exit(main())
