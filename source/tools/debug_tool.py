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
            rsync_cmd = Template("rsync --recursive --compress --exclude='user-repo' " +
                                 "--exclude='sys-repo' $user@$node:$fds_root :/corefiles $local_path")
            rsync_cmd = rsync_cmd.substitute(password=self._config.password,
                                             user=self._config.user,
                                             node=self._config.nodes[node]['ip'],
                                             fds_root=self._config.nodes[node]['fds_root'],
                                             local_path=os.path.join(self._store_dir, node))

            # pexpect regex for password prompt
            pass_expect_re = re.compile(r'.* password:')
            cont_expect_re = re.compile(r'.* continue connecting (yes/no)?')
            # call pexpect to fire off the rsync process        
            proc = pexpect.spawn(rsync_cmd)
            res = proc.expect([cont_expect_re, pass_expect_re])
            
            if res == 0:
                proc.sendline('yes')
                proc.expect(pass_expect_re)
                proc.sendline(self._config.password)
            elif res == 1:
                proc.sendline(self._config.password)

            # Busy wait for a few minutes -- we need to be sure
            # everything is finished before we go tarring everything
            # up
            print "File transfer in progress, this may take a while..."
            while(proc.isalive()):
                pass
                
            proc.close()
            
    def do_pack(self, out):
        '''
        Do the packaging by first calling self.__collect, and then packing all
        of the collected files in a single tgz file.

        Returns:
          string - path to the compressed tarball
        '''

        # Collect all of the files locally
        self.__collect()

        print "Remote files collected, packaging..."

        # Make the tar'd paths look nice
        xform = ''
        if self._store_dir[-1] != '/':
            xform = 's,' + self._store_dir[1:] + '/' + ',,'
        else:
            xform = 's,' + self._store_dir[1:] + ',,'
            
        # Tar them up
        # tar --create --gzip --file
        tar_cmd = ['tar', '--transform=' + xform, '--create', '--gzip', '--file', out, self._store_dir + '/*']
        FNULL = open(os.devnull, 'w')
        proc = subprocess.check_call(' '.join(tar_cmd), shell=True, stdout=FNULL, stderr=FNULL)
        FNULL.close()
        
        return out
        
    def __str__(self):
        ''' 
        For pretty printing.
        '''
        return self._store_dir

if __name__ == "__main__":

    
    parser = argparse.ArgumentParser()
    parser.add_argument('config_file', help='Config file to read information from.')
    parser.add_argument('-d', help='Path where collected cores/logs/etc should be stored.', default='~/')
    parser.add_argument('-o', help='Name of output bundle.',
                        default='debug_pack-'+datetime.datetime.now().isoformat('_').replace(':', '.')+'.tgz')
    parser.add_argument('--gdb', help='Run GDB on cores in specified directory, and output basic information to text file.')

    args = parser.parse_args()

    cc = ClusterConfig(args.config_file)
    db = DebugBundle(cc, args.d)
    res = db.do_pack(args.o)
    print "Debug package created:", res
