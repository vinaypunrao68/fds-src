#!/usr/bin/env python

'''
pack_debug.py -- Pack up all files necessary for debugging a system post crash.
'''

import pexpect
import argparse
import datetime
import subprocess
import os
import sys
import getpass
import re
from string import Template

class ClusterConfig(object):

    def __init__(self, config_path):

        # Dictionary of nodes
        # A node will be in the form:
        # node[node_id] = {
        #                   ip: 'xxx.xxx.xxx.xxx',
        #                   fds_root: '/path/to/fds/root'
        #                 }
        self._nodes = {}
        self._user = None
        self._password = None

        self.__parse_config(config_path)
        

    @property
    def nodes(self):
        ''' Get node information from config. '''
        return self._nodes

    @property
    def user(self):
        ''' Get username from config. '''
        return self._user

    @property
    def password(self):
        ''' Get password from config. '''
        return self._password
    

    def __parse_config(self, config_path):
        ''' Parses the config file and sets appropriate 
        class variables.

        Returns:
          None
        '''

        node_re = re.compile('\[(node(.+))\]')
        ip_re = re.compile('ip\s+=\s+(.+)')
        fds_root_re = re.compile('fds_root\s+=\s+(.+)')
        user_re = re.compile('user_name\s+=\s+(.+)')
        pass_re = re.compile('password\s+=\s+(.+)')
    
        open_node = None
        with open(config_path) as fh:
            for line in fh:
                # Check for user name
                res = user_re.match(line)
                if res is not None:
                    self._user = res.group(1)
                    continue
                # Check for password
                res = pass_re.match(line)
                if res is not None:
                    self._password = res.group(1)
                    continue
                # Check for node before IP or fds_root
                res = node_re.match(line)
                if res is not None:
                    self._nodes[res.group(1)] = {}
                    open_node = res.group(1)
                    continue
                # Verify we found a node first
                if open_node is not None:
                    # Get IP address of node
                    res = ip_re.match(line)
                    if res is not None:
                        self._nodes[open_node]['ip'] = res.group(1)
                        continue
                    # Get fds_root of node
                    res = fds_root_re.match(line)
                    if res is not None:
                        self._nodes[open_node]['fds_root'] = res.group(1)
                        continue


    def __str__(self):
        acc = ''
        for node in self._nodes.keys():
            acc += ("Node = " + str(node) + " Vals = " +
                    str(self._nodes[node]) + '\n')

        return acc


class DebugBundle(object):

    def __init__(self, config, store_dir):
        '''
        Create a DebugBundle
        '''
        
        assert config.user is not None, "User must be set!"
        assert config.password is not None, "Password must be set!"
        assert config.nodes != {}, "No nodes found!"

        self._store_dir = store_dir
        self._config = config
        self._f_out = None
        
    def __collect(self):
        '''
        Collect all of the cores/logs/etc from each of the nodes defined in
        config.
        
        Returns:
          None
        '''        
        # Create the output directory
        for node in self._config.nodes.keys():
            # Create a folder for this node
            try:
                os.makedirs(os.path.join(self._store_dir, node))
                
            except OSError as e:
                if e.errno != 17:
                    print e

            print "Collecting debug files from remote nodes..."

            # Call rsync to get all of the files
            rsync_cmd = Template("rsync --recursive --copy-links --compress --exclude='user-repo' " +
                                 "--exclude='sys-repo' $user@$node:$fds_root :/corefiles $local_path")
            rsync_cmd = rsync_cmd.substitute(password=self._config.password,
                                             user=self._config.user,
                                             node=self._config.nodes[node]['ip'],
                                             fds_root=self._config.nodes[node]['fds_root'],
                                             local_path=os.path.join(self._store_dir, node))


            self.__expect_rsync(rsync_cmd, self._config.password)
            
    def __expect_rsync(self, rsync_cmd, password=None):
        '''
        Uses pexpect to call rsync and handle the potential outcomes.
        Params:
          rsync_cmd - (string) full command to execute rsync with proper options
          password - (string) Password to send to rsync (defaults to None
                     which will prompt the user for a password.
        Returns:
          None
        '''
        # pexpect regex for password prompt
        pass_expect_re = re.compile(r'.* password:')
        cont_expect_re = re.compile(r'.* continue connecting (yes/no)?')
        # call pexpect to fire off the rsync process        
        proc = pexpect.spawn(rsync_cmd)
        res = proc.expect([cont_expect_re, pass_expect_re])

        if password is None:
            password = getpass.getpass()
        
        if res == 0:
            proc.sendline('yes')
            proc.expect(pass_expect_re)
            proc.sendline(password)
        elif res == 1:
            proc.sendline(password)

        # Busy wait for a few minutes -- we need to be sure
        # everything is finished before we go tarring everything
        # up
        print "File transfer in progress, this may take a while..."
        while(proc.isalive()):
            pass
                
        proc.close()
        print "File transfer complete, remote files collected."

    def do_pack(self, out):
        '''
        Do the packaging by first calling self.__collect, and then packing all
        of the collected files in a single tgz file.

        Returns:
          string - path to the compressed tarball
        '''

        out = os.path.join(self._store_dir, out)
        
        # Collect all of the files locally
        self.__collect()
        # Remove files from the remote node
        self.__do_delete(local=False)
        
        print "Packaging files..."

        # Make the tar'd paths look nice
        xform = ''
        if self._store_dir[-1] != '/':
            xform = 's,' + self._store_dir[1:] + '/' + ',,'
        else:
            xform = 's,' + self._store_dir[1:] + ',,'
            
        # Tar them up
        # tar --create --gzip --file
        tar_cmd = ['tar', '--transform=' + xform, '--create', '--gzip',
                   '--file', out, self._store_dir + '/*']
        FNULL = open(os.devnull, 'w')
        proc = subprocess.check_call(' '.join(tar_cmd), shell=True, stdout=FNULL, stderr=FNULL)
        FNULL.close()

        # Store the location/name of the output file
        self._f_out = out

        self.__do_delete()

        return out

    def push(self, push_to):
        '''
        Pushes the tarball to the specified path using rsync.
        Params:
          push_to - (string) path to push the bundle to
        Returns:
          None
        '''

        print "Pushing tarball to", push_to

        if self._f_out is None:
            print "Error: no output file found!"
            sys.exit(1)
        
        rsync_cmd = Template("rsync $f_name $push_path")
        rsync_cmd = rsync_cmd.substitute(f_name=self._f_out, push_path=push_to)
    
        # If we're pushing to a remote path
        if '@' in push_to:
            # Call __expect_rsync with a None password to indicate that
            # the user should be prompted. This password may be different
            # than the password in the config file.
            self.__expect_rsync(rsync_cmd)

        else:
            subprocess.call(rsync_cmd.split())

        print "Push complete..."

    def __do_delete(self, local=True):
        '''
        Will delete .core files, logs, etc from remote machine after they have
        been collected, and tar'd on the local machine.
        '''
        # Local delete or remote delete?
        if local:
            # Do local delete
            # Nuke all of the node directories in self._store_dir
            print "Cleaning temporary storage directories..."

            for node in self._config.nodes.keys():
                rm_cmd = ['rm', '-rf', os.path.join(self._store_dir, node)]
                subprocess.call(rm_cmd)

        else:
            # Do remote delete
            # TODO: connect to remote machine and selectively delete .core files and logs
            print "Cleaning remote nodes... (currently non-functional)q"
    

    def __str__(self):
        ''' 
        For pretty printing.
        '''
        return self._store_dir

if __name__ == "__main__":

    
    parser = argparse.ArgumentParser(description='Package, compress, and manage debug files.')
    subparsers = parser.add_subparsers(help='A sub-command must be specified.', dest='subp')
    
    pack_parser = subparsers.add_parser('package',
                                        help='Sub-command to package up debug files.')
    pack_parser.add_argument('-f',
                             help='Config file to read information from.',
                             required=True)
    
    pack_parser.add_argument('--push',
                             help='Push generated core bundle to specified location. (default: %(default)s)',
                             default=False, metavar='user@host:path',
                             nargs='?',
                             const='coke.formationds.com:/media/cores/cores/')
    pack_parser.add_argument('-d',
                             help='Path where collected cores/logs/etc should be stored. (default: %(default)s',
                             default='~/', metavar='/path/to/dest')
    pack_parser.add_argument('-o',
                             help='Name of output bundle. (default: %(default)s)',
                             default='debug_pack-'+datetime.datetime.now().isoformat('_').replace(':', '.')+'.tgz',
                             metavar='filename')

    debug_parser = subparsers.add_parser('debug',
                                         help='Sub-command to perform debugging actions on core files.')
    debug_parser.add_argument('--gdb',
                              help='Run GDB on cores in specified directory, and output basic information to text file.',
                              metavar='command')

    args = parser.parse_args()

    if args.subp == 'package':
        # Build a package
        cc = ClusterConfig(args.f)
        db = DebugBundle(cc, args.d)
        res = db.do_pack(args.o)    
        print "Debug package created:", res

        # Send package somewhere?
        if args.push is not False:
            db.push(args.push)
            
    elif args.subp == 'debug':
        # Run the gdb command and log the bt
        print "Function currently unimplemented"
        pass

    print "It has been a pleasure serving you!"
