#!/usr/bin/env python

# Copyright 2014 by Formation Data Systems, Inc.
#
import os, errno, sys
import subprocess, pdb
import paramiko, time
from scp import SCPClient
from multiprocessing import Process
from contextlib import contextmanager

@contextmanager
def pushd(new_dir):
    prev_dir = os.getcwd()
    os.chdir(new_dir)
    yield
    os.chdir(prev_dir)

def mkdir_p(path):
    try:
        os.makedirs(path)
    except OSError as exc:
        if exc.errno == errno.EEXIST:
            if os.path.isdir(path):
                pass
            else:
                print path, " exists and it's not a directory"
                exit(1)
        else:
            raise

# --------------------------------------------------------------------------------------
###
# FDS environment data derived from the closet FDS source tree or FDS Root run time.
#
class FdsEnv(object):
    def __init__(self, _root):
        self.env_cdir      = os.getcwd()
        self.env_fdsSrc    = ''
        self.env_install   = False
        self.env_fdsRoot   = _root + '/'
        self.env_user      = 'root'
        self.env_password  = 'passwd'
        self.env_fdsDict   = {
            'debug-base': 'Build/linux-x86_64.debug/',
            'package'   : 'fds.tar',
            'pkg-sbin'  : 'test',
            'pkg-tools' : 'tools',
            'root-sbin' : 'sbin'
        }

        tmp_dir = self.env_cdir
        while tmp_dir != "/":
            if os.path.exists(tmp_dir + '/Build/mk-scripts'):
                self.env_fdsSrc = tmp_dir
                break
            tmp_dir = os.path.dirname(tmp_dir)

        if self.env_fdsSrc == "":
            # Assume node installation, assign the same as fds-root
            self.env_install = True
            self.env_fdsSrc  = self.env_fdsRoot

        self.env_fdsSrc = self.env_fdsSrc + '/'

    def shut_down(self):
        subprocess.call(['pkill', '-9', 'AMAgent'])
        subprocess.call(['pkill', '-9', 'Mgr'])
        subprocess.call(['pkill', '-9', 'platformd'])
        subprocess.call(['pkill', '-9', '-f', 'com.formationds.web.om.Main'])

    def cleanup(self):
        self.shut_down()
        os.chdir(self.env_fdsSrc)
        subprocess.call([self.env_fdsSrc + 'tools/fds', 'clean'])

    def cleanup_install(self, debug = True, fds_bin = True):
        with pushd(self.get_build_dir(debug, fds_bin)):
            try:
                subprocess.call(['rm', self.get_pkg_tar(debug)])
            except: pass

    ###
    # Return $fds_src/Build/<build_target> if fds_bin is True
    #        $fds_src/../Build/<build_target> to get 3nd party codes.
    #
    def get_build_dir(self, debug, fds_bin = True):
        if self.env_install:
            return self.env_fdsRoot
        if fds_bin:
            return self.env_fdsSrc + self.env_fdsDict['debug-base'];
        return self.env_fdsSrc + '../' + self.env_fdsDict['debug-base'];

    ###
    # Return the absolute path of the tar ball.
    #
    def get_pkg_tar(self, debug = True, fds_bin = True):
        return self.get_build_dir(debug, fds_bin) + self.env_fdsDict['package']

    ###
    # Return $fds_src/Build/<build_target>/bin if fds_bin == True or
    #        $fds_src/../Build/<build_target/bin to get 3nd party binaries.
    #
    def get_bin_dir(self, debug, fds_bin = True):
        return self.get_build_dir(debug, fds_bin) + 'bin'

    ###
    # Return $fds_src/Build/<build_target>/lib if fds_bin == True or
    #        $fds_src/Build/<build_target>/lib to get 3nd party libraries.
    #
    def get_lib_dir(self, debug, fds_bin = True):
        return self.get_build_dir(debug, fds_bin) + 'lib'

    ###
    # In fds_src, return $fds_src/test directory.
    #   In installation dir, return fds_root/sbin directory.
    #
    def get_sbin_dir(self):
        if self.env_install:
            return self.env_fdsRoot + 'sbin'
        return self.env_fdsSrc + 'test'

    def get_config_dir(self):
        if self.env_install:
            return self.env_fdsRoot + 'etc'
        return self.env_fdsSrc + 'config'

    def get_fds_root(self):
        return self.env_fdsRoot

    def get_fds_source(self):
        return self.env_fdsSrc

####
# Bring the same enviornment as FdsEnv but operate on a remote node.
# Execute command on a remote node.
#
class FdsRmtEnv(FdsEnv):
    def __init__(self, root, verbose = None):
        super(FdsRmtEnv, self).__init__(root)
        self.env_ssh_clnt  = None
        self.env_scp_clnt  = None
        self.env_verbose   = verbose
        self.env_rmt_host  = None

        # Overwrite the parent class to make it behaves like fds-root installation.
        #
        self.env_fdsSrc    = root
        self.env_install   = True

        self.env_ldLibPath = ("export LD_LIBRARY_PATH=" +
                self.get_fds_root() + 'lib:'
                '/usr/local/lib:/usr/lib/jvm/java-8-oracle/jre/lib/amd64; '
                'export PATH=$PATH:' + self.get_fds_root() + 'bin; ')

    ###
    # Open ssh connection to the remote node.
    #
    def ssh_connect(self, host, port = 22, user = None, passwd = None):
        try:
            # Connect to the remote host
            client = paramiko.SSHClient()
            client.load_system_host_keys()
            client.set_missing_host_key_policy(paramiko.AutoAddPolicy())

            user   = self.env_user if user == None else user
            passwd = self.env_password if passwd == None else passwd

            self.env_rmt_host = host
            client.connect(host, port, user, passwd)
        except Exception, e:
            print "*** Caught execption %s: %s" % (e.__class__, e)
            try:
                client.close()
            except: pass
            sys.exit(1)

        self.env_ssh_clnt = client
        return client

    ###
    # Open an scp connection to the remote host.
    #
    def scp_open(self, *args, **kwargs):
        if self.env_ssh_clnt is None:
            self.ssh_connect(args, kwargs)

        self.env_scp_clnt = SCPClient(self.env_ssh_clnt.get_transport())
        return self.env_scp_clnt 

    def scp_copy(self, local, remote, *args, **kwargs):
        if self.env_scp_clnt is None:
            self.scp_open(args, kwargs)
        self.env_scp_clnt.put(local, remote)

    ###
    # Execute command to a remote node through ssh client.
    #
    def ssh_exec(self, 
                 cmd, 
                 wait_compl = False, 
                 fds_bin = False, 
                 output = False, 
                 return_stdin = False):

        if fds_bin:
            cmd_exec = (self.env_ldLibPath + 'cd ' + self.get_fds_root() +
                        'bin; ulimit -c unlimited; ulimit -n 12800; ./' + cmd)
        else:
            cmd_exec = cmd

        if self.env_verbose:
            if self.env_verbose['verbose']:
                print "Running remote command on %s:" % self.env_rmt_host, cmd_exec

            if self.env_verbose['dryrun'] == True:
                print "...not execute in dryrun mode"
                return 0

        stdin, stdout, stderr = self.env_ssh_clnt.exec_command(cmd_exec)
        channel = stdout.channel
        status  = 0 if wait_compl == False else channel.recv_exit_status()
        if output == True:
            for line in stdout.read().splitlines():
                print("[%s] %s" % (self.env_rmt_host, line))
            for line in stderr.read().splitlines():
                print("[%s Error] %s" % (self.env_rmt_host, line))

        return_line = None
        if return_stdin and wait_compl:
            return_line = stdout.read()

        stdin.close()
        stdout.close()
        stderr.close()

        if return_stdin and wait_compl:
            return status, return_line.rstrip()
        return status

    ###
    # Execute command from $fds_root/bin directory.  Prefix with needed stuffs.
    #
    def ssh_exec_fds(self, cmd, wait_compl = False):
        return self.ssh_exec(cmd, wait_compl, True)

    def ssh_close(self):
        env_ssh_clnt.close()

    def get_host_name(self):
        return "" if self.env_rmt_host is None else self.env_rmt_host

    ###
    # Setup core files and other stuffs.
    #
    def ssh_setup_env(self, cmd):
        self.ssh_exec(cmd +
            '; echo "%e-%p.core" | sudo tee /proc/sys/kernel/core_pattern ' +
            '; sudo sysctl -w "kernel.core_pattern=%e-%p.core"' +
            '; sysctl -p' +
            '; echo "1" >/proc/sys/net/ipv4/tcp_tw_reuse' +
            '; echo "1" >/proc/sys/net/ipv4/tcp_tw_recycle')

###
# Package FDS tar ball and unpackage it to a remote host
#
class FdsPackage:
    def __init__(self, p_env):
        self.p_env = p_env
        if self.p_env.get_fds_root() == self.p_env.get_fds_source():
            print "You must run this command under FDS source tree"
            sys.exit(1)

    ###
    # Create a tar ball from FDS source tree.
    #
    def package_tar(self, debug = True):
        with pushd(self.p_env.get_build_dir(debug)):
            tar_ball = self.p_env.get_pkg_tar(debug)

            self.p_env.cleanup_install(debug)
            sys.stdout.write("Creating FDS tar ball: ")
            sys.stdout.write(tar_ball)

            subprocess.call(['tar', 'cf', tar_ball, 'bin'])
            subprocess.call(['tar', 'rf', tar_ball, 'lib/java'])
            subprocess.call(['tar', 'rf', tar_ball, 'lib/admin-webapp'])
            sys.stdout.write('..')
            with pushd(self.p_env.get_config_dir()):
                subprocess.call(['tar', 'rf', tar_ball, 'etc'])
                sys.stdout.write('..')

            #with pushd(self.p_env.get_fds_source()):
            #    subprocess.call(['tar', 'rf', tar_ball, 'test'])

            with pushd(self.p_env.get_fds_source()):
                subprocess.call(['tar', 'rf', tar_ball, 'tools'])

            with pushd(self.p_env.get_build_dir(debug, fds_bin = False)):
                subprocess.call(['cp', '../../leveldb-1.12.0/libleveldb.so.1', 'lib'])
                subprocess.call(['tar', 'rf', tar_ball, 'lib'])

            sys.stdout.write('..\n')

    ###
    # Untar the tar ball to the local host under fds_root argument.
    #
    def package_untar(self, debug = True):
        mkdir_p(self.p_env.get_fds_root())
        with pushd(self.p_env.get_fds_root()):
            tar_ball = self.p_env.get_pkg_tar(debug)
            subprocess.call(['tar', 'xf', tar_ball])

    ###
    # Install the tar ball to a remote node using default user/passwd from env obj.
    #
    def package_install(self, rmt_ssh, local_path = None):
        print "Copying the tar ball package to %s at: %s" % (
            rmt_ssh.get_host_name(), self.p_env.get_fds_root())

        if local_path is None:
            local_path = self.p_env.get_pkg_tar()

        print "Installing the package from:", local_path
        rmt_ssh.ssh_exec('mkdir -p ' + self.p_env.get_fds_root())
        rmt_ssh.scp_copy(local_path, self.p_env.get_fds_root())

        print "Unpacking the tar ball package to ", rmt_ssh.get_host_name()
        rmt_ssh.ssh_exec('cd ' + self.p_env.get_fds_root() +
                         '; tar xf ' + self.p_env.env_fdsDict['package'] +
                         '; cd ' + self.p_env.get_fds_root() +
                         '; rm ' + self.p_env.env_fdsDict['package'] +
                         '; cp -rf ' + self.p_env.env_fdsDict['pkg-sbin'] + ' ' +
                                   self.p_env.env_fdsDict['root-sbin'] +
                         '; rm -rf ' + self.p_env.env_fdsDict['pkg-sbin'] +
                         '; cp -rf ' + self.p_env.env_fdsDict['pkg-tools'] + ' ' +
                                   self.p_env.env_fdsDict['root-sbin'] +
                         '; rm -rf ' + self.p_env.env_fdsDict['pkg-tools']  +
                         '; mkdir -p var/logs')
