#!/usr/bin/python
import os, sys, re
import Pyro4
import shlex
import tempfile
import subprocess as sub
from optparse import OptionParser

class Executor(object):
    def __init__(self):
        super( Executor, self ).__init__()
        self.proc_map = {}

    def add_proc(self, pid, proc, fileout, fileerr):
        self.proc_map[pid] = {"proc" : proc, "fileout" : fileout, "fileerr" : fileerr }

    def get_proc(self, pid):
        return self.proc_map[pid]

    def exec_cmd_simple(self, cmd):
        args = shlex.split(cmd)
        print args
        output = sub.check_output(args, stderr=sub.STDOUT)
        return output

    def exec_cmd(self, cmd):
        args = shlex.split(cmd)
        fileout, fnameout = tempfile.mkstemp(prefix = "out")
        fileerr, fnameerr = tempfile.mkstemp(prefix = "err")
        p = sub.Popen(args, stdout = fileout, stderr = fileerr)
        pid = p.pid
        print fnameout, fnameerr
        self.add_proc(pid, p, (fileout, fnameout), (fileerr, fnameerr))
        return pid

    def terminate(self, pid):
        print "Killing",pid
        info = self.get_proc(pid)
        info["proc"].kill()
        os.close(info["fileout"][0])
        os.close(info["fileerr"][0])
        return info["fileout"][1], info["fileerr"][1]
        
    def ping(self):
        return "pong"
        
def main():
    parser = OptionParser()
    parser.add_option("-d", "--node-name", dest = "node_name", default = "node", help = "Node name")
    parser.add_option("-n", "--name-server-ip", dest = "ns_ip", default = "10.1.10.102", help = "IP/hostname of the name server")
    parser.add_option("-p", "--name-server-port", dest = "ns_port", type = "int", default = 47672, help = "Port of the name server")
    parser.add_option("-s", "--server-ip", dest = "s_ip", default = "10.1.10.18", help = "IP/hostname of the server")
    (options, args) = parser.parse_args()


    executor = Executor()
    daemon=Pyro4.Daemon(host = options.s_ip)                                # make a Pyro daemon
    ns=Pyro4.locateNS(host = options.ns_ip, port = options.ns_port)         # find the name server
    uri=daemon.register(executor)   
    ns.register("%s.executor" % options.node_name, uri)  
    
    print "Ready."
    daemon.requestLoop()                  # start the event loop of the server to wait for calls

if __name__ == "__main__":
    main()
