"""
Class to manage the physical processes of an FDS node.
"""

import abc
import subprocess
import psutil
import shlex
import os
import paramiko
import unittest
import unittest.mock
import pdb

class ProcessCtrl(abc.ABC):
    """
    Abstract base class for process controllers. Local and remote process controllers should inherit from this.
    """
     # R - Running
     # D - Uninterruptable sleep
     # S - Interruptable sleep
     # Z - Zombie
     # T - Stopped
    PROCESS_STATUS = {
        "S" : "Sleeping",
        "R" : "Running",
        "D" : "Sleeping",
        "Z" : "Zombie",
        "T" : "Stopped"
    }

    COMPONENT_TO_BINARY = {
        'sm' : 'StorMgr',
        'dm' : 'DataMgr',
        'om' : 'com.formationds.om.Main',
        'pm' : 'platformd',
        'am' : 'com.formationds.am.Main',
        'xdi' : 'bare_am'
    }

    def __init__(self, service_name, ip, fds_port, fds_root):
        self._ip = ip
        self._base_port = fds_port
        self._fds_root = fds_root
        self._bin_dir = os.path.join(self._fds_root, 'bin')
        self._logs_dir = os.path.join(self._fds_root, 'var', 'logs')
        self._pid = None

        self._binary_name = ProcessCtrl.COMPONENT_TO_BINARY[service_name.lower()]


    @abc.abstractmethod
    def kill_process(self):
        raise NotImplementedError("Class {} does not implement a kill_process method.".format(self.__class__.__name__))

    @abc.abstractmethod
    def get_status(self):
        raise NotImplementedError("Class {} does not implement a get_status method".format(self.__class__.__name__))


    def _find_pid_from_ps(self, ps_in):
        """
        Finds a PID from the output of PS.
        :param input: A string containing the contents of the ps command.
        :return: A PID on success; None on failure
        """
        pid = None
        # The output of check_output is a string of bytes, decode it as ascii and split on newlines
        for line in ps_in.split("\n"):
            if (self._binary_name == "com.formationds.om.Main" and self._binary_name in line) or \
                    (self._binary_name in line and "platform_port=" + str(self._base_port) in line):

                assert pid is None, "Found multiple PIDs that match binary name {} and platform port {}" \
                    .format(self._binary_name, self._base_port)

                # Expected formatting is: UID PID PPID C STIME TTY TIME CMD
                pid = int(line.split()[1])

        return pid


    def _find_status_from_ps(self, ps_in):
        """
        Gets the status from the output of ps -p <pid> -o state --no-headers
        :param ps_in: The output of the ps -p <pid> -o state --no-headers command
        :return One of: Sleeping, Running, Zombie, Stopped; None on failure
        """

        if type(ps_in) == type(b''):
            ps_in = ps_in.decode('ascii')

        return self.PROCESS_STATUS.get(ps_in.strip(), None)

class LocalProcessCtrl(ProcessCtrl):
    """
    Class to provide local process control via process pid.
    """

    def __init__(self, service_name, ip='127.0.0.1', fds_port=7000, fds_root="/fds"):
        """
        Returns a new LocalProcessCtrl instance.

        :param service_name: The name of the service to control (pm, om, am, dm, sm).
        :param ip: The IP address of the node (default: localhost)
        :param fds_port: The base port for FDS (default: 7000)
        :param fds_root: The root directory of the FDS install (default: /fds)

        :return: New LocalProcessCtrl instance
        """
        super().__init__(service_name, ip, fds_port, fds_root)

        # start_process method only available for the PM/OM, all other services should be started via REST calls
        if service_name == "pm" or service_name == "om":
            self.start_process = lambda: subprocess.Popen([self._bin_dir + "/" + self._binary_name]).pid

    def kill_process(self, on_success=None, on_failure=None):
        """
        End the process with a SIGKILL. This is not a clean shutdown. Status of a process can be retrieved with
        the get_status() method.

        :return: None
        """
        # If we don't already have a PID for this service, get it
        if self._pid is None:
            self._get_pid()

        if self._pid is None:
            return

        cmd = shlex.split("kill -9 {}".format(self._pid))

        # Should be no response on this one
        subprocess.check_output(cmd)


    def get_status(self):
        """
        Get the status of the current service's process.

        :return: Returns one of: running, sleeping, stopped, zombie, PID_NOT_FOUND; None on failure
        """

        if self._pid is None:
            self._get_pid()

        # If we still have no PID just return none
        if self._pid is None:
            return "PID_NOT_FOUND"

        cmd = shlex.split("ps -p {} -o state --no-headers".format(self._pid))
        o = subprocess.check_output(cmd)

        return self._find_status_from_ps(o)

    def _get_pid(self):
        """
        Get the PID for the specified binary/base port combo.
        :return: PID on success, None on failure
        """
        # List everything, and we'll check for self._binary_name and self._base_port when we filter the output

        cmd = shlex.split("ps -efww --no-headers")
        output = subprocess.check_output(cmd, universal_newlines=True)
        pid = self._find_pid_from_ps(output)
        self._pid = pid

        return pid


class RemoteProcessCtrl(ProcessCtrl):


    def __init__(self, service_name, ip='127.0.0.1', ssh_port=22, ssh_user=None, ssh_password=None,
                 fds_port=7000, fds_root="/fds"):
        """
        Returns a new RemoteProcessCtrl instance.

        :param service_name: The name of the service to control (pm, om, am, dm, sm).
        :param ssh_port: SSH port to connect to
        :param ssh_user: Username to authenticate via SSH with
        :param ssh_password: Password to use in conjunction with ssh_user to authenticate with the remote system
        :param ip: The IP address of the node (default: localhost)
        :param fds_port: The base port for FDS (default: 7000)
        :param fds_root: The root directory of the FDS install (default: /fds)

        :return: New RemoteProcessCtrl instance
        """

        super().__init__(service_name, ip, fds_port, fds_root)

        # Setup paramiko stuffs
        self._remote = paramiko.SSHClient()
        self._remote.load_system_host_keys()
        self._remote.set_missing_host_key_policy(paramiko.AutoAddPolicy())
        if ssh_user is not None and ssh_password is not None:
            self._remote.connect(ip, port=ssh_port, username=ssh_user, password=ssh_password)
        else:
            self._remote.connect(ip, port=ssh_port)

        self._pid = None

        # start_process method only available for the PM/OM, all other services should be started via REST calls
        if service_name == "pm" or service_name == "om":
            #TODO: Define more robust start_process method to make more consistent behavior
            self.start_process = lambda: self._remote.exec_command([self._bin_dir + "/" + self._binary_name])

    def get_status(self):
        """
        Get the status of the specified process.
        :return: One of: running, sleeping, zombie, stopped, or PID_NOT_FOUND; None on failure
        """

        if self._pid is None:
            self._get_pid()

        # If we still have no PID just return none
        if self._pid is None:
            return "PID_NOT_FOUND"

        cmd = "ps -p {} -o state --no-headers".format(self._pid)
        i, o, e = self._remote.exec_command(cmd)

        return self._find_status_from_ps(o.read())

    def kill_process(self):
        """
        End the process with a SIGKILL. This is not a clean shutdown. Status of a process can be retrieved with
        the get_status() method.
        :return: None
        """

        # If we don't already have a PID for this service, get it
        if self._pid is None:
            self._get_pid()

        if self._pid is None:
            return

        cmd = "kill -9 {}".format(self._pid)

        # No output from this one
        i, o, e = self._remote.exec_command(cmd)

    def _get_pid(self):
        """
        Send the ps command via paramiko to the remote node and gets the PID for the service. Set self._pid.
        :return The PID of the service on success; None on failure
        """

        cmd = "ps -efww --no-headers"
        # returns stdin, stdout, stderr
        i, o, e = self._remote.exec_command(cmd)

        pid = self._find_pid_from_ps("".join(o.readlines()))
        self._pid = pid

        return pid


##############################
#          TESTS             #
##############################

MOCK_PID_OUTPUT = """
root      5514   795  0 13:23 ?        00:00:00 /sbin/dhclient -d -sf /usr/lib/NetworkManager/nm-dhcp-client.action -pf /run/sendsigs.omit.d/network-manager.dhclient-eth0.pid -lf /var/lib/NetworkManager/dhclient-3e63a553-22b4-4647-85ea-ab4e8fc82244-eth0.lease -cf /var/lib/NetworkManager/dhclient-eth0.conf eth0
influxdb  6361     1  0 15:55 pts/9    00:00:00 /usr/bin/influxdb -pidfile /opt/influxdb/shared/influxdb.pid -config /opt/influxdb/shared/config.toml
root      6372     1  0 15:55 pts/9    00:00:00 ./platformd --fds-root=/fds
root      6407     1  0 15:55 pts/9    00:00:00 /bin/bash ./orchMgr --fds-root=/fds
root      6410  6407 10 15:55 pts/9    00:00:07 /opt/fds-deps/embedded/jre/bin/java -Xmx2048M -Xms20M -cp :../lib/java/* com.formationds.om.Main --foreground --fds-root=/fds
root      6499  6372 10 15:55 pts/9    00:00:05 /home/brian/Documents/fds-src/master/source/Build/linux-x86_64.debug/bin/StorMgr --foreground --fds.pm.platform_uuid=7805375779937640128 --fds.common.om_ip_list=127.0.0.1 --fds.pm.platform_port=7000 --fds-root=/fds
root      6501  6372  9 15:55 pts/9    00:00:04 /home/brian/Documents/fds-src/master/source/Build/linux-x86_64.debug/bin/DataMgr --foreground --fds.pm.platform_uuid=7805375779937640128 --fds.common.om_ip_list=127.0.0.1 --fds.pm.platform_port=7000 --fds-root=/fds
root      6502  6372 10 15:55 pts/9    00:00:05 /home/brian/Documents/fds-src/master/source/Build/linux-x86_64.debug/bin/bare_am --foreground --fds.pm.platform_uuid=7805375779937640128 --fds.common.om_ip_list=127.0.0.1 --fds.pm.platform_port=7000 --fds-root=/fds
root      7033  6372 62 15:56 pts/9    00:00:01 java -classpath /fds//lib/java/* -agentlib:jdwp=transport=dt_socket,server=y,suspend=n,address=14777 com.formationds.am.Main --fds.pm.platform_uuid=7805375779937640128 --fds.common.om_ip_list=127.0.0.1 --fds.pm.platform_port=7000 --fds-root=/fds
brian     7045  2236  0 15:56 pts/9    00:00:00 ps -efww --no-headers
brian     8388   680  0 06:00 pts/19   00:00:03 emacs -nw tools/fds
root     10479  1102  0 00:06 ?        00:00:00 sshd: brian [priv]
brian    10560 10479  0 00:06 ?        00:00:03 sshd: brian@pts/2
brian    10561 10560  0 00:06 pts/2    00:00:00 -zsh
brian    10578 10561  0 00:06 pts/2    00:00:00 tmux -2 at
root     12727     1  0 Aug17 ?        00:00:00 upstart-udev-bridge --daemon
root     12730     1  0 Aug17 ?        00:00:00 /lib/systemd/systemd-udevd --daemon
root     12886     1  0 Aug17 ?        00:00:00 /lib/systemd/systemd-logind
root     12969     1  0 Aug17 ?        00:00:01 /usr/sbin/cups-browsed
root     13028     1  0 Aug17 ?        00:00:01 /usr/sbin/cupsd -f
root     16941     1  0 Aug25 ?        00:07:00 redis-server *:6379
brian    18370 29385  0 06:35 pts/6    00:00:07 emacs -nw test/testsuites/testcases/TestFDSSysVerify.py
brian    18371 18370  0 06:35 pts/15   00:00:00 /usr/bin/python -i
syslog   18602     1  0 Aug17 ?        00:00:57 rsyslogd
brian    21122  2229  0 Aug17 pts/17   00:00:00 -zsh
root     25308     2  0 Aug23 ?        00:03:24 [kworker/0:1]
brian    27165  2229  0 Aug17 pts/18   00:00:00 -zsh
root     27926     2  0 Aug17 ?        00:00:00 [xfsalloc]
"""

class TestLocalCtrl(unittest.TestCase):

    def testDefaults(self):
        """
        Tests that default object creation is sane.
        """
        # Just create default objects and verify that all of the defaults are correct
        procs = ['pm', 'om', 'sm', 'dm', 'am']
        for p in procs:
            proc = LocalProcessCtrl(p)
            self.assertEqual(proc._ip, '127.0.0.1')
            self.assertEqual(proc._base_port, 7000)
            self.assertEqual(proc._fds_root, '/fds')
            self.assertEqual(proc._bin_dir, os.path.join('/fds', 'bin'))
            self.assertEqual(proc._logs_dir, os.path.join('/fds', 'var', 'logs'))

            # Only OM and PM should have a start_process method, everything else is started by the PM
            if p == 'om' or p == 'pm':
                self.assertTrue(hasattr(proc, 'start_process'))
            else:
                self.assertFalse(hasattr(proc, 'start_process'))


    def testNonDefaults(self):
        """
        Test object creation with non-default options
        """

        # Create objects with non-default settings
        procs = ['pm', 'om', 'sm', 'dm', 'am']

        for p in procs:
            proc = LocalProcessCtrl(p, '192.168.1.1', 8000, '/fds/test')

            self.assertEqual(proc._ip, '192.168.1.1')
            self.assertEqual(proc._base_port, 8000)
            self.assertEqual(proc._fds_root, '/fds/test')
            self.assertEqual(proc._bin_dir, os.path.join('/fds', 'test', 'bin'))
            self.assertEqual(proc._logs_dir, os.path.join('/fds', 'test', 'var', 'logs'))


    @unittest.mock.patch('process.subprocess.check_output', return_value=MOCK_PID_OUTPUT)
    def testGetPID(self, *args, **kwargs):
        """
        Test get _get_pid method
        """
        proc = LocalProcessCtrl("sm")
        self.assertEqual(proc._get_pid(), 6499)

    @unittest.mock.patch('__main__.LocalProcessCtrl', autospec=True)
    def testGetStatus(self, mock_lpc):
        """
        Test get_status()
        """
        proc = mock_lpc("sm")
        proc._find_status_from_ps.return_value = "S"
        res = proc.get_status()
        print(res)
        self.assertEqual(res, "Sleeping")

if __name__ == "__main__":
    # Run tests
    unittest.main()