#!/usr/bin/python
#
# Copyright 2014 Formation Data Systems, Inc.
#
import optparse
import pdb
import ServiceMgr
import ServiceConfig

verbose = False
debug   = False
section = None

if __name__ == '__main__':
    #
    # Parse command line options
    #
    parser = optparse.OptionParser("usage: %prog [options]")
    parser.add_option("-f", "--file", dest="config_file",
                      help="configuration file", metavar="FILE")
    parser.add_option("-v", "--verbose", action="store_true",
                      dest="verbose", help="enable verbosity")
    parser.add_option("-b", "--debug", action="store_true",
                      dest="debug", help="enable debug breaks")
    parser.add_option("-u", "--up", action="store_true",
                      dest="up", help="bring up cluster")
    parser.add_option("-d", "--down", action="store_true",
                      dest="down", help="bring down cluster")
    parser.add_option("-c", "--clean-shutdown", action="store_true",
                      dest="shutdown", help="clean shutdown cluster")
    parser.add_option("-s", "--section", dest="section",
                      help="specific section to bring up",
                      metavar="section name")
    parser.add_option("-i", "--ssh_user", default=None, dest="ssh_user",
                      help="ssh user name")
    parser.add_option("-p", "--ssh_passwd", default=None, dest="ssh_passwd",
                      help="ssh password")
    parser.add_option("-k", "--ssh_key", default=None, dest="ssh_key",
                      help="ssh key file")
    parser.add_option("-S", "--source_path", default=None, dest="source_path",
                      help="path to source")
    (options, args) = parser.parse_args()
    cfgFile = options.config_file
    verbose = options.verbose
    debug = options.debug
    up = options.up
    down = options.down
    shutdown = options.shutdown
    section = options.section
    userName = options.ssh_user
    userPasswd = options.ssh_passwd
    sshKey = options.ssh_key
    sourcePath = options.source_path

    #
    # Load the configuration files
    #
    bu = ServiceConfig.TestBringUp(cfgFile, verbose, debug)
    bu.loadCfg(userName, userPasswd, sshKey)

    #
    # Bring up all the services
    #
    if up == True:
        if section != None:
            result = bu.bringUpSection(section)
            if result != 0:
                print "Failed to bring up section %s" % (section)
            else:
                print "Brought up section %s" % (section)
        else:
            result = bu.bringUpNodes()
            if result != 0:
                print "Failed to bring up all nodes"
            else :
                print "Brought up all nodes"
                result = bu.bringUpClients()
                if result != 0:
                    print "Failed to bring up all clients"
                else :
                    print "Brought up all clients"
                    result = bu.bringUpPols()
                    if result != 0:
                        print "Failed to bring up all policies"
                    else :
                        print "Brought up all policies"
                        result = bu.bringUpVols()
                        if result != 0:
                            print "Failed to bring up all volumes"
                        else :
                            print "Brought up all volumes"

    if down == True:
        result = bu.bringDownClients()
        if result != 0:
            print "Failed to bring down all clients"
        else :
            print "Brought down all clients"
        result = bu.bringDownNodes()
        if result != 0:
            print "Failed to bring down all nodes"
        else:
            print "Brought down all nodes"

    if shutdown == True:
        # For now, require specific section to be given
        assert(section != None)
        # For now, only bring down the SM service
        result = bu.shutdownSection(section, "SM")
        if result != 0:
            print "Failed to cleanly shutdown %s" % (section)
        else:
            print "Cleanly shutdown section %s" % (section)
