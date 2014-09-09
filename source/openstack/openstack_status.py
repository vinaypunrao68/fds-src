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

    # Openstack specific configurations
    # list all openstack services
    # identity - keystone
    ops.append_output("keystone service-list")

    # image - glance
    ops.append_output("glance --version")
    ops.append_output("glance image-list")

    # block - cinder
    ops.append_output("cinder service-list")

    # compute - nova
    ops.append_output("nova service-list")


    # object storage - swift (TBD)
    # Network (TBD)
    # Orchestration (TBD)
    # Telemetry (TBD)

    ops.print_summary()

    return 0

if __name__ == '__main__':
    sys.exit(main())
