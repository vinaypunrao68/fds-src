#!/usr/bin/env python
#
# Copyright 2014 Formation Data Systems, Inc.
#
import fdslib.FdsSetup as inst
import optparse
import sys

if __name__ == '__main__':
    parser = optparse.OptionParser('usage: %prog [options]')
    parser.add_option('--fds-root', dest = 'fds_root',
                      default = '/fds',  help = 'FDS Root directory')
    parser.add_option('--host', dest = 'host',
                      help = 'Remote host to install FDS package')
    parser.add_option('--mode', dest = 'bin_mode',
                      help = 'debug|release', action = 'store_true')
    parser.add_option('--op', dest = 'op_mode',
                      default = 'pkg',
                      help = 'pkg|install|all')

    (options, args) = parser.parse_args()
    env = inst.FdsEnv(options.fds_root)
    pkg = inst.FdsPackage(env)

    print "Using %s as fds-root" % options.fds_root
    if options.op_mode == 'all' or options.op_mode == 'pkg':
        pkg.package_tar()
        print "Installing to local fds-root at: ", options.fds_root
        pkg.package_untar()
   
    if options.op_mode == 'all' or options.op_mode == 'install':
        if options.host is None:
            print "You need to pass hostname to --host option"
            sys.exit(1)

        print "Installing to %s at: %s" % (options.host, options.fds_root)
        rmt = inst.FdsRmtEnv(options.fds_root)
        rmt.ssh_connect(options.host)
        pkg.package_install(rmt)
