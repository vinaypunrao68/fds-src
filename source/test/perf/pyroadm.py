import os, sys, re
import time
import Pyro4
import paramiko
import SocketServer
import multiprocessing
import threading
import Queue
import tempfile
import shutil
import datetime
import requests
import counter_server
import subprocess
import shlex
import is_fds_up
from optparse import OptionParser

def get_local_ip():
    cmd = "ifconfig | grep 10.1.10 | awk \'{print $2}\' |  awk -F \':\' \'{print $2}\'"
    print cmd
    output = subprocess.check_output(cmd, shell=True)
    return output.rstrip("\n")

def start_ns(node, port):
    cmd = "ssh %s 'nohup python -Wignore -m Pyro4.naming --host %s --port %s 2> pyro_ns.err > pyro_ns.out &'" % (node, node, port)
    subprocess.call(cmd, shell=True)

def start_pyro(node, hostname, ns_ip, ns_port):
    cmd = "scp cmd-server.py %s:" % node
    subprocess.call(cmd, shell=True)
    cmd = "ssh %s 'nohup python cmd-server.py -d %s -n %s -p %s -s %s 2> cmd_server.err > cmd_server.out &'" % (node, hostname, ns_ip, ns_port, node)
    print cmd
    subprocess.call(cmd, shell=True)

def pyro_start(ns_ip, ns_port, nodes):
    start_ns(ns_ip, ns_port)
    for name, ip in nodes.iteritems():
        start_pyro(ip, name, ns_ip, ns_port) 

def get_ns_pids(node):
    cmd = "ssh %s 'ps aux|grep Pyro4.naming| grep -v grep' | awk '{print $2}'" % node
    output = subprocess.check_output(cmd, shell=True)
    pids = [int(x) for x in filter(lambda x: len(x) > 0, re.split("\n",output))]
    return pids

def get_pyro_pids(node):
    cmd = "ssh %s 'ps aux|grep cmd-server.py |grep -v grep' | awk '{print $2}'" % node
    output = subprocess.check_output(cmd, shell=True)
    
    pids = [int(x) for x in filter(lambda x: len(x) > 0, re.split("\n",output))]
    return pids

def kill_pids(node, pids):
    if len(pids) > 0:
        cmd = "ssh %s 'kill -9 %s'" % (node, reduce(lambda x, y: str(x) + " " + str(y), pids))
        subprocess.call(cmd, shell=True)

def pyro_stop(ns, nodes):
    nodes = nodes.values()
    pids = get_ns_pids(ns)
    kill_pids(ns, pids)
    for n in nodes:
        pids = get_pyro_pids(n)
        kill_pids(n, pids)

def is_ns_up(ns):
    cmd = "ssh %s 'ps aux|grep Pyro4.naming| grep -v grep' | wc -l" % ns
    up = int(subprocess.check_output(cmd, shell=True).rstrip("\n"))
    print "NS on: ", ns, "up:", up
    if up == 1:
        return True
    else:
        return False

def is_pyro_up(node):
    cmd = "ssh %s 'ps aux|grep cmd-server.py |grep -v grep' | wc -l" % node
    up = int(subprocess.check_output(cmd, shell=True).rstrip("\n"))
    print "Pyro on: ", node, "up:", up
    if up == 1:
        return True
    else:
        return False

def pyro_check(ns, nodes):
    nodes = nodes.values()
    check = is_ns_up(ns)
    for n in nodes:
        check = is_pyro_up(n) and check
    return check

def main():
    parser = OptionParser()
    parser.add_option("-s", "--start", dest = "start", action="store_true", default=False,
                      help = "Start Pyro")
    parser.add_option("-d", "--down", dest = "down", action="store_true", default=False,
                      help = "Bring down Pyro")
    parser.add_option("-c", "--check", dest = "check", action="store_true", default=False,
                      help = "Check if Pyro is up")
    (options, args) = parser.parse_args()

    options.nodes = {"luke" : "10.1.10.222", "han" : "10.1.10.139", "c3po" : "10.1.10.221", "chewie" : "10.1.10.80"}
    options.ns_ip = "10.1.10.139"
    options.ns_port = 47672

    print options, args

    if options.check:
        outcome = pyro_check(options.ns_ip, options.nodes)
        print "all:", int(outcome)
    if options.down:
        pyro_stop(options.ns_ip, options.nodes)
    if options.start:
        pyro_start(options.ns_ip, options.ns_port, options.nodes)

if __name__ == "__main__":
    main()
