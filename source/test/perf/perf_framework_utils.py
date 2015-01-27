#!/usr/bin/python
import os, re, sys
import time
import socket
import paramiko
import datetime
import unittest
import subprocess
import shlex

class PerfLogger(object):
    DEBUG = 10
    INFO = 20
    WARNING = 30
    ERROR = 40
    CRITICAL = 50

    def __init__(self, name, level):
        self.name = name
        self.level = level
    
    @classmethod
    def get_level_from_str(self, name):
        return {"DEBUG" : self.DEBUG, "INFO" : self.INFO, "WARNING" : self.WARNING, "ERROR" : self.ERROR, "CRITICAL" : self.CRITICAL}[name]

    def debug(self, msg):
        if self.level >= self.DEBUG: 
            print "DEBUG ::", self.name, "::", msg
    def info(self, msg):
        if self.level >= self.INFO:
            print "INFO ::", self.name, "::", msg
    def warning(self, msg):
        if self.level >= self.WARNING:
            print "WARNING ::", self.name, "::", msg
    def error(self, msg):
        if self.level >= self.ERROR:
            print "ERROR ::", self.name, "::", msg
    def critical(self, msg):
        if self.level >= self.CRITICAL:
            print "CRITICAL ::", self.name, "::", msg

logger = PerfLogger("utils", PerfLogger.DEBUG)


class RemoteExecutor:
    def __init__(self, node, options):
        self.options = options
        self.options.logger.debug("Starting remote executor on " + node)
        self.node = node
        self.proc_map = {}

    def execute(self, cmd):
        output = ssh_exec(self.node, "mktemp")
        stdout = output.rstrip("\n")
        output = ssh_exec(self.node, "mktemp")
        stderr = output.rstrip("\n")
        ssh_exec(self.node, "chmod 755 " + stdout)
        ssh_exec(self.node, "chmod 755 " + stderr)
        output = ssh_exec(self.node, "%s > %s 2> %s & echo $!" % (cmd, stdout, stderr))
        pid = int(output.rstrip("\n"))
        self.proc_map[pid] = {"fileout" : stdout, "fileerr" : stderr}
        return pid

    def terminate(self, pid, directory):    
        self.options.logger.debug("Terminating pid: " + str(pid))

        output = ssh_exec(self.node, "kill -9 " + str(pid))

        info = self.proc_map[pid]
        time.sleep(5)
        transfer_file(self.node, directory + "/stdout", info["fileout"], "get")
        transfer_file(self.node, directory + "/stderr", info["fileerr"], "get")

def ssh_exec(node, cmd):
    logger.debug("ssh_exec on " + node + " -> " + cmd)
    ssh = paramiko.SSHClient()
    ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    ssh.connect(node, username='root', password='passwd')
    (stdin, stdout, stderr) = ssh.exec_command(cmd)
    output = stdout.read()
    ssh.close()
    return output

def transfer_file(node, local_f, remote_f, mode):
    logger.debug("transfer_file " + node + " " + local_f + " " + remote_f + " " + mode)
    ssh = paramiko.SSHClient()
    ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    ssh.connect(node, username='root', password='passwd')
    ftp = ssh.open_sftp()
    if mode == "get":
        ftp.get(remote_f, local_f)
    elif mode == "put":
        ftp.put(local_f, remote_f)
    else:
        raise "Error: transfer mode can be put or get"
    ftp.close()
    ssh.close()

def loc_exec(cmd):
    logger.debug(cmd)
    output = subprocess.check_output(shlex.split(cmd))
    return output

def get_ip(hostname):
    return socket.gethostbyname(hostname)


def get_my_ip():
    return get_ip('localhost')

def test_to_str(test):
    test_str = ""
    for k, v in test.iteritems():
        if k != "injector":
            test_str += str(k) + ":" + str(v) + "."
    test_str += "test"
    return test_str

def get_results_dir(base = "results", test = None):
    directory0 = base + "_" + str(datetime.date.today()) + "_" + test_to_str(test)
    directory = directory0
    cntr = 1
    while os.path.exists(directory):
        directory = directory0  + "_" + str(cntr)
        cntr += 1
    os.makedirs(directory)
    return directory



# class NonBlockingExecutor(object):
#     def __init__(self, node):
#         self.node = node
# 
#     def work(self, cmd, node):
#         output = ssh_exec(cmd, node)
# 
#     def execute(self, cmd):
#         ssh_exec("%s > %s 2> %s & echo $!" % (cmd, stdout, stderr))
# 
#     def terminate(self):
#         # need to kill shell command
#         self.thread.

class TestSequenceFunctions(unittest.TestCase):
    def test_get_ip(self):
        self.assertEqual(get_ip('luke'),'10.1.10.222')

if __name__ == '__main__':
    unittest.main()
    
