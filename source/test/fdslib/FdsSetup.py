#!/usr/bin/env python

# Copyright 2014 by Formation Data Systems, Inc.
#
import os, errno, sys, pwd
sys.path.append("/opt/fds-deps/embedded/lib/python2.7/site-packages")
import logging
import subprocess
import shlex
import paramiko
import datetime
from scp import SCPClient
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
    def __init__(self, _root, _verbose=None, _install=False, _fds_source_dir='', _test_harness=False):
        log = logging.getLogger(self.__class__.__name__ + '.' + "__init__")

        self.env_cdir      = os.getcwd()
        self.env_fdsSrc    = _fds_source_dir
        self.env_install   = _install
        self.env_fdsRoot   = _root + '/'
        self.env_verbose   = _verbose
        self.env_host      = None
        self.env_user      = 'root'
        self.env_password  = 'passwd'
        self.env_sudo_password = 'dummy'
        self.env_test_harness = _test_harness
        self.env_fdsDict   = {
            'debug-base': 'Build/linux-x86_64.debug/',
            'package'   : 'fds.tar',
            'pkg-sbin'  : 'test',
            'pkg-tools' : 'tools',
            'root-sbin' : 'sbin'
        }

        self.env_ldLibPath = ("export LD_LIBRARY_PATH=" +
                              self.get_fds_root() + 'lib:'
                              '/usr/local/lib:/opt/fds-deps/embedded/jre/lib/amd64:'
                              '/opt/fds-deps/embedded/lib; '
                              'export PATH=/opt/fds-deps/embedded/jre/bin:/opt/fds-deps/embedded/bin:$PATH:' + self.get_fds_root() + 'bin; ')

        # Try to determine an FDS source directory if specified as empty.
        if self.env_fdsSrc == "":
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

        log.debug("fds_root: %s" % (self.env_fdsRoot))
        log.debug("fds_src_dir: %s" % (self.env_fdsSrc))

    ###
    # Return $fds-root if we installed from an FDS package, otherwise
    #        $fds_src/Build/<build_target> if fds_bin is True
    #        $fds_src/../Build/<build_target> to get 3nd party codes.
    #
    def get_build_dir(self, debug, fds_bin = True):
        if self.env_install:
            return self.env_fdsRoot
        if fds_bin:
            return self.env_fdsSrc + self.env_fdsDict['debug-base']
        return self.env_fdsSrc + '../' + self.env_fdsDict['debug-base']

    ###
    # Return $fds-root/var/logs if we installed from an FDS package, otherwise
    #        $fds_src/Build/<build_target>/bin.
    #
    def get_log_dir(self):
        if self.env_install:
            return self.env_fdsRoot + 'var/logs'
        else:
            return self.get_bin_dir(debug=False)

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

    def get_tools_dir(self):
        if self.env_install:
            return self.env_fdsRoot + 'sbin'
        return self.env_fdsSrc + 'tools'

    def get_config_dir(self):
        if self.env_install:
            return self.env_fdsRoot + 'etc'
        return self.env_fdsSrc + 'config/etc'

    def get_memcheck_dir(self):
        if self.env_install:
            return self.env_fdsRoot + 'memcheck'
        return self.env_fdsSrc + 'config/memcheck'

    def get_fds_root(self):
        return self.env_fdsRoot

    def get_fds_source(self):
        return self.env_fdsSrc

    def get_host_name(self):
        return "" if self.env_host is None else self.env_host

####
# Bring the same environment as FdsEnv but
# execute commands on the local node.
#
class FdsLocalEnv(FdsEnv):
    def __init__(self, root, verbose=None, install=True, test_harness=False):
        super(FdsLocalEnv, self).__init__(root, _verbose=verbose, _install=install, _test_harness=test_harness)

    ###
    # Open connection to the local node.
    #
    def local_connect(self, user = None, passwd = None, sudo_passwd = None):
        log = logging.getLogger(self.__class__.__name__ + '.' + "local_connect")

        if user is not None:
            self.env_user = user
        if passwd is not None:
            self.env_password = passwd
        if sudo_passwd is not None:
            self.env_sudo_password = sudo_passwd
        if self.env_sudo_password is None:
            # No sudo password supplied. In this case we require that
            # the login name of the process's real user ID match
            # the configured user.
            if pwd.getpwuid(os.getuid())[0] != self.env_user:
                log.error("No sudo password given for process owner %s to use configured user %s. "
                          "Please restart with -d <sudo_password> or specify 'sudo_password' in the config file" %
                          (pwd.getpwuid(os.getuid())[0], self.env_user))
                sys.exit(1)
            else:
                self.env_sudo_password = sudo_passwd

        self.env_host = 'localhost'

        return self

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
    # Execute command on local node.
    #
    def local_exec(self,
                 cmd,
                 wait_compl = False,
                 fds_bin = False,
                 fds_tools = False,
                 output = False,
                 return_stdin = False,
                 cmd_input=None):
        log = logging.getLogger(self.__class__.__name__ + '.' + "local_exec")

        # We need to modify the command to use the credentials that have been configured.
        # This usage of 'sudo' will get the password from stdin (rather than the terminal device)
        # and ignore any cached credentials.
        if fds_bin and self.env_install:
            cmd_exec = (self.env_ldLibPath + 'cd ' + self.get_fds_root() + 'bin; '
                        'ulimit -c unlimited; ulimit -n 12800; ' +
                        'sudo -S -k -u %s ' % self.env_user + cmd)
        else:
            cmd_exec = "sudo -S -k -u %s " % self.env_user + cmd

        if self.env_verbose:
            if self.env_verbose['verbose']:
                if cmd_input is not None:
                    log.info("Running local command: %s [<< %s]" % (cmd_exec, cmd_input))
                else:
                    log.info("Running local command: %s" % (cmd_exec))

            if self.env_verbose['dryrun']:
                log.info("...not executed in dryrun mode")
                return 0

        # Split the command into a list of strings as prefered by subprocess.Popen()
        call_args = shlex.split(cmd_exec)

        # For FDS binaries in a development environment or for "FDS tools", we need to switch to that
        # directory for execution.
        cur_dir = os.getcwd()
        if fds_bin and not self.env_install:
            os.chdir(self.get_bin_dir(debug=True))
        elif fds_tools:
            os.chdir(self.get_tools_dir())

        p = subprocess.Popen(call_args, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

        # Probably p.communicate() is forcing a wait for the process to complete
        # regardless of wait_compl's setting.
        if cmd_input is not None:
            # Watch for a sudo password of "dummy". In that case we'll assume that the environment
            # will not request a password for sudo and so execute the command without providing it
            # as input.
            if self.env_sudo_password == "dummy":
                stdout, stderr = p.communicate(cmd_input + "\n")
            else:
                stdout, stderr = p.communicate(self.env_sudo_password + "\n" + cmd_input + "\n")
        else:
            stdout, stderr = p.communicate(self.env_sudo_password + "\n")

        if wait_compl:
            p.wait()
        else:
            log.info("Not waiting.")

        status = p.returncode

        # For FDS binaries in a development environment or for "FDS tools", we need to switch back
        # to our original directory.
        if (fds_bin and not self.env_install) or fds_tools:
            os.chdir(cur_dir)

        if stderr is not None:
            for line in stderr.splitlines():
                # These are stderr "warnings" and "errors" we wish to ignore.
                # sudo prompts show up in stderr.
                if line.startswith("[sudo] password for "):
                    # If the line does not extend past ':', ignore it.
                    # Otherwise, there's more information there than just the prompt.
                    if line.__len__() <= line.find(":")+2:
                        continue
                    else:
                        prompt, colon, line = line.partition(":")

                if 'log4j:WARN' in line:
                    continue

                if 'Content is not allowed in prolog.' in line:
                    continue

                # From fsdconsole.py
                if 'InsecureRequestWarning' in line:
                    continue

                # From fdsconsole.py
                if 'InsecurePlatformWarning' in line:
                    continue

                if status == 0:
                    log.warning("Shell reported status 0 from command execution but stderr "
                                "contains unexpected output as follows. Forcing status to -1.")
                    status = -1

                log.warning("[{} stderr] {}".format(self.env_host, line))

        if output and (stdout is not None):
            if status != 0:
                if len(stdout) > 0:
                    log.warning("Non-zero status from shell command execution, {}, "
                                "or unrecognized stderr output.".format(status))
                    log.warning("stdout contents as follows.")
                for line in stdout.splitlines():
                    log.warning("[{} stdout] {}".format(self.env_host, line))

        return_line = None
        if return_stdin and wait_compl and (stdout is not None):
            return_line = stdout

        if return_stdin and wait_compl:
            return status, return_line.rstrip()

        return status

    ###
    # Execute command from $fds_root/bin directory.  Prefix with needed stuffs.
    #
    def local_exec_fds(self, cmd, wait_compl = False):
        return self.local_exec(cmd, wait_compl, True)

    ###
    # Execute command and wait for result. We'll also log
    # output in this case.
    #
    def exec_wait(self, cmd, return_stdin=False, cmd_input=None, wait_compl=True, fds_bin=False, fds_tools=False, output=True):
        return self.local_exec(cmd, wait_compl=wait_compl, fds_bin=fds_bin, fds_tools=fds_tools, output=output, return_stdin=return_stdin,
                               cmd_input=cmd_input)

    def local_close(self):
        pass

    ###
    # Setup core files and other stuffs.
    #
    def setup_env(self, cmd):
        if (cmd is not None) and (cmd != ''):
            cmd = cmd + ';'

        # Removed this for [WIN-1081], this doesn't work anyway.
        #self.local_exec(cmd +
            #' echo "%e-%p.core" | sudo tee /proc/sys/kernel/core_pattern ' +
            #'; sudo sysctl -w "kernel.core_pattern=%e-%p.core"' +
            #'; sysctl -p' +
            #'; echo "1" >/proc/sys/net/ipv4/tcp_tw_reuse' +
            #'; echo "1" >/proc/sys/net/ipv4/tcp_tw_recycle')

####
# Bring the same environment as FdsEnv but operate on a remote node.
# Execute command on a remote node.
# Always treat a remote as a root installation, i.e. outside of a
# development environment (hence, _install=True).
#
class FdsRmtEnv(FdsEnv):
    def __init__(self, root, verbose=None, test_harness=False):
        super(FdsRmtEnv, self).__init__(root, _verbose=verbose, _install=True, _test_harness=test_harness)
        self.env_ssh_clnt  = None
        self.env_scp_clnt  = None

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

            self.env_host = host
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
                 fds_tools = False,
                 output = False,
                 return_stdin = False,
                 cmd_input = None):
        log = logging.getLogger(self.__class__.__name__ + '.' + "ssh_exec")

        if fds_bin:
            if self.env_test_harness:
                cmd_exec = (self.env_ldLibPath + 'cd ' + self.get_fds_root() +
                            'bin; ulimit -c unlimited; ulimit -n 12800; ' + cmd)
            else:
                cmd_exec = (self.env_ldLibPath + 'cd ' + self.get_fds_root() +
                            'bin; ulimit -c unlimited; ulimit -n 12800; ./' + cmd)
        elif fds_tools:
            if self.env_test_harness:
                cmd_exec = (self.env_ldLibPath + 'cd ' + self.get_tools_dir() +
                            '; ulimit -c unlimited; ulimit -n 12800; ' + cmd)
            else:
                cmd_exec = (self.env_ldLibPath + 'cd ' + self.get_tools_dir() +
                            '; ulimit -c unlimited; ulimit -n 12800; ./' + cmd)
        else:
            cmd_exec = cmd

        if self.env_verbose:
            if self.env_verbose['verbose']:
                if self.env_test_harness:
                    log.info("Running remote command on %s: %s" % (self.env_host, cmd_exec))
                else:
                    print("Running remote command on %s: %s" % (self.env_host, cmd_exec))

            if self.env_verbose['dryrun'] == True:
                if self.env_test_harness:
                    log.info("...not executed in dryrun mode")
                else:
                    print("...not executed in dryrun mode")
                return 0

        stdin, stdout, stderr = self.env_ssh_clnt.exec_command(cmd_exec)
        if cmd_input is not None:
            # TODO(Greg): Seems not to be working. Execution behaves as if input still wanted.
            log.debug("cmd_input: %s" % cmd_input)
            stdin.write(cmd_input)
            stdin.flush()

        channel = stdout.channel
        status  = 0 if wait_compl == False else channel.recv_exit_status()

        if self.env_test_harness:
            for line in stderr.read().splitlines():
                # These are stderr "warnings" and "errors" we wish to ignore.
                if 'log4j:WARN' in line:
                    continue

                if 'Content is not allowed in prolog.' in line:
                    continue

                # From fsdconsole.py
                if 'InsecureRequestWarning' in line:
                    continue

                if status == 0:
                    log.warning("Shell reported status 0 from command execution but stderr "
                                "contains unexpected output as follows. Forcing status to -1.")
                    status = -1

                log.warning("[{} stderr] {}".format(self.env_host, line))

            if output == True:
                if status != 0:
                    firstTime = True
                    for line in stdout.read().splitlines():
                        if firstTime:
                            log.warning("Non-zero status from shell command execution, {}, "
                                        "or unrecognized stderr output.".format(status))
                            log.warning("stdout contents as follows.")
                            firstTime = False

                        log.warning("[{} stdout] {}".format(self.env_host, line))
        else:
            if output == True:
                for line in stdout.read().splitlines():
                    print("[%s] %s" % (self.env_host, line))
                for line in stderr.read().splitlines():
                    print("[%s Error] %s" % (self.env_host, line))

        return_line = None
        # It appears to me, GCarter, that return_stdin is being used
        # here where output is intended. At any rate, I made local_exec()
        # conform to this usage. If corrected here, then please correct
        # there as well.
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

    def exec_wait(self, cmd, return_stdin = False, cmd_input=None, wait_compl=True, fds_bin=False, fds_tools=False, output=True):
        return self.ssh_exec(cmd, wait_compl=wait_compl, fds_bin=fds_bin, fds_tools=fds_tools, output=output, return_stdin=return_stdin,
                             cmd_input=cmd_input)

    def ssh_close(self):
        self.env_ssh_clnt.close()

    ###
    # Setup core files and other stuffs.
    #
    def setup_env(self, cmd):
        if (cmd is not None) and (cmd != ''):
            cmd = cmd + ';'

        # Removed this for [WIN-1081], this doesn't work anyway.
        # self.ssh_exec(cmd +
            # ' echo "%e-%p.core" | sudo tee /proc/sys/kernel/core_pattern ' +
            # '; sudo sysctl -w "kernel.core_pattern=%e-%p.core"' +
            # '; sysctl -p' +
            # '; echo "1" >/proc/sys/net/ipv4/tcp_tw_reuse' +
            # '; echo "1" >/proc/sys/net/ipv4/tcp_tw_recycle')

###
# Package FDS tar ball and unpackage it to a remote host
#
class FdsPackage:
    def __init__(self, p_env):
        self.p_env = p_env
        if self.p_env.get_fds_root() == self.p_env.get_fds_source():
            print "You must run this command under FDS source tree"
            sys.exit(1)

    def create_tar_installer (self, debug = True):
        ''' Create a tarball using the make_tar.sh script
        '''
        package_name = 'fdsinstall-' + datetime.datetime.now().strftime ("%Y%m%d-%H%M") + ".tar.gz"
        with pushd(self.p_env.get_fds_source() + "/tools/install"):
            subprocess.call(['./make_tar.sh', package_name])

        return '/tmp/' + package_name

    ###
    # Create a tar ball from FDS source tree.
    #
    def package_tar(self, debug = True):
        print "Use of this function is deprecated"
        raise DeprecationWarning
        '''
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
        '''

    ###
    # Untar the tar ball to the local host under fds_root argument.
    #
    def package_untar(self, debug = True):
        print "Use of this function is deprecated"
        raise DeprecationWarning
        '''
        mkdir_p(self.p_env.get_fds_root())
        with pushd(self.p_env.get_fds_root()):
            tar_ball = self.p_env.get_pkg_tar(debug)
            subprocess.call(['tar', 'xf', tar_ball])
        '''

    ###
    # Install the tar ball to a remote node using default user/passwd from env obj.
    #
    def package_install(self, rmt_ssh, local_path = None):
        print "Use of this function is deprecated"
        raise DeprecationWarning
        '''
        print "Copying the tar ball package to %s at: %s" % (
            rmt_ssh.get_host_name(), self.p_env.get_fds_root())

        if local_path is None:
            local_path = self.p_env.get_pkg_tar()

        print "Installing the package from:", local_path
        rmt_ssh.ssh_exec('mkdir -p ' + self.p_env.get_fds_root())
        rmt_ssh.scp_copy(local_path, self.p_env.get_fds_root())

        print "Unpacking the tar ball package to ", rmt_ssh.get_host_name()
        status = rmt_ssh.ssh_exec('cd ' + self.p_env.get_fds_root() +
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

        return status
        '''
