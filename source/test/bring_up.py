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
    parser.add_option("-s", "--section", dest="section",
                      help="specific section to bring up",
                      metavar="section name")
    (options, args) = parser.parse_args()
    cfgFile = options.config_file
    verbose = options.verbose
    debug = options.debug
    up = options.up
    down = options.down
    section = options.section

    #
    # Load the configuration files
    #
    bu = ServiceConfig.TestBringUp(cfgFile, verbose, debug)
    bu.loadCfg()

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
