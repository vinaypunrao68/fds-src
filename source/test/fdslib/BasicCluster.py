#!/usr/bin/env python

# Copyright 2014 by Formation Data Systems, Inc.
#

import os
import time

# List of proceses expected to be running on all nodes.
all_nodes_process_list = ['platformd', 'DataMgr', 'StorMgr']

# List of processes expected to be running on AM/OM nodes.
process_list = ['om.Main', 'am.Main', 'bare_am'] + all_nodes_process_list

# Display setting for status command
process_status_field_list = 'euser,pid,pcpu,pmem,nlwp,stat,bsdstart,bsdtime'
process_status_header =        '       Process   User        PID %CPU %MEM Thrd STAT START  TIME'
process_status_output_format = '       %-10s%-9s%6s%5.1f%5.1f%5d %-5s%-6s %-s'
#                                      Process  Pid     %mem Thd     Start
#                                           User   %CPU          STAT    TIME

class basic_cluster:
    def __init__ (self, nodes, am_nodes, cli_cmd):
        self.nodes = nodes
        self.am_nodes = am_nodes
        self.cli_cmd = cli_cmd

    def status (self, verbose = False, monitor = False):
        ''' Report on the status of each process that is expected to be running on each node

            verbose:  if true, process details will be report
            monitor:  if true, a single line of output is generated (for use by a monitoring tool)
        '''

        return_buffer = ''     # only used in monitor mode
        return_val = 0         # only used in monitor mode

        if verbose and not monitor:
            print "\n", process_status_header

        for node in self.nodes:
            is_am_node = False
            node_name_displayed = False

            if not monitor:
               print "%s" % (node.nd_conf_dict['node-name'])

            # Determine if this is an AM node
            for am_node in self.am_nodes:
                if am_node.nd_conf_dict['fds_node'] == node.nd_conf_dict['node-name']:
                    is_am_node = True
                    break

            for proc in process_list:
                if proc in all_nodes_process_list or is_am_node:
                    (status, pid) = node.nd_agent.ssh_exec ('pgrep -o -f %s' % (proc), wait_compl = True, return_stdin = True)
                    if 0 == status:             # Successful ssh and pgrep
                        if not monitor:
                            if verbose:
                                (status, vals) = node.nd_agent.ssh_exec ('ps --pid %s -o %s | tail -1' % (pid, process_status_field_list), wait_compl = True, return_stdin = True)
                                arr = vals.split()
                                print process_status_output_format % (proc, arr[0], arr[1], float (arr[2]), float (arr[3]), int (arr[4]), arr[5], arr[6], arr[7])
                            else:
                                print "       %-10s  %s" % (proc + ':', 'Up')
                    else:                       # something failed, figure out what to do with it.
                        return_val = 2          # Use 2 to indicate critical in Nagios
                        if monitor:
                            if not node_name_displayed:
                                node_name_displayed = True
                                return_buffer += node.nd_conf_dict['node-name'] + ":"
                            return_buffer += proc + "=Down "
                        else:
                            print "       %-10s  %s" % (proc + ':', 'Down')

        if monitor:
            return return_val, return_buffer

    def install (self, tar_file, verbose = False):
        '''
        '''
        for node in self.nodes:
            if verbose:
                print "Copying the file: %s to %s" % (tar_file, node.nd_agent.get_host_name())
            node.nd_agent.ssh_exec ('rm -rf ./fdsinstall', wait_compl=True)
            node.nd_agent.scp_copy (tar_file, "~/.")
            node.nd_agent.ssh_exec ('tar -zxf ' + os.path.basename (tar_file), wait_compl=True)
            node.nd_agent.ssh_exec ('rm -f ' + os.path.basename (tar_file), wait_compl=True)

        return 0, "All Okay"

    def deploy (self, verbose = False):
        '''
        '''
        for node in self.nodes:
            if verbose:
                print "Deploying FDS software upgrades on %s" % (node.nd_agent.get_host_name())

            node.nd_agent.ssh_exec (cmd='cd fdsinstall && ./fdsinstall.py -o 2', return_stdin = True, wait_compl = True)
            node.nd_agent.ssh_exec (cmd='cd fdsinstall && ./fdsinstall.py -o 3', return_stdin = True, wait_compl = True)
            node.nd_agent.ssh_exec (cmd='cd fdsinstall && ./fdsinstall.py -o 4', return_stdin = True, wait_compl = True)
        return 0, "All Okay"

