#!/usr/bin/python
import os,sys,re
import time
import threading as th
#from multiprocessing import Process, Queue
from optparse import OptionParser
import tempfile
#from Queue import *
#import httplib
#import requests
import random
import paramiko

class Execute:
    def __init__(self):
        self.nodes = {}
    def add_node(self, node, ip):
        self.nodes[node] = ip

    def execute(channel, command):
        command = 'echo $$; exec ' + command
        stdin, stdout, stderr = channel.exec_command(command)
        pid = int(stdout.readline())
        return pid, stdin, stdout, stderr


    def exec_cmd_bg(self, node, cmd):
        ssh = paramiko.SSHClient()
        ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
        ssh.connect(self.nodes[node], username='root', password='passwd')
        handle, name = tempfile.mkstemp()
        
        os.close(handle)
        cmd = cmd + " > /dev/null 2>&1 & ; echo $!"
        stdin, stdout, stderr = ssh.exec_command(cmd)
        pid = int(stdout.readline())
        ssh.close()
        print pid
        return pid
        

    def exec_cmds(self, node, cmds):
        ssh = paramiko.SSHClient()
        ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
        ssh.connect(self.nodes[node], username='root', password='passwd')
        stdin, stdout, stderr = ssh.exec_command(cmds[0])
        #type(stdin)
        print stdout.readlines()
        #stdin.write('lol\n')
        #stdin.flush()
        ssh.close()
        return stdout



def main():
    ex = Execute()
    ex.add_node("node1","10.1.10.17")
    task_args = ("node1","iostat -p -k -d 1")
    t = th.Thread(None,ex.exec_cmd_bg,"task-"+str(0), task_args)
    t.start()

    t.join()

if __name__ == "__main__":
    parser = OptionParser()
    #parser.add_option("-t", "--threads", dest = "threads", type = "int", default = 1, help = "Number of threads")
    #parser.add_option("-n", "--num-requests", dest = "n_reqs", type = "int", default = 1, help = "Number of requests per thread")
    #parser.add_option("-T", "--type", dest = "req_type", default = "PUT", help = "PUT/GET/DELETE")
    #parser.add_option("-s", "--file-size", dest = "file_size", type = "int", default = 4096, help = "File size in bytes")
    #parser.add_option("-F", "--num-files", dest = "num_files", type = "int", default = 10000, help = "File size in bytes")
    #parser.add_option("-v", "--num-volumes", dest = "num_volumes", type = "int", default = 1, help = "Number of volumes")
    (options, args) = parser.parse_args()
    main()
 
