#!/usr/bin/python
import os, re, sys
from optparse import OptionParser
import perf_framework_utils as utils

agents={
    "pm" : "platformd",
    "om" : "com\.formationds\.om", 
    "am" : "bare_am", 
    "xdi" : "com\.formationds\.am", 
    "sm" : "StorMgr", 
    "dm" : "DataMgr", 
}

def get_pid_table(nodes):
    pid_table = {}
    for n in nodes:
       for a in agents:
            cmd = "ps aux | egrep %s | grep -v egrep | grep -v defunct " % agents[a]
            output = utils.ssh_exec(n, cmd)
            tokens = output.split()
            if len(tokens) > 2:
                if not n in pid_table:
                    pid_table[n] = {}
                pid = int(tokens[1])
                pid_table[n][a] = pid
    return pid_table

def get_info(agent, n="localhost"): 
    cmd = "ps aux | egrep %s | grep -v egrep | grep -v defunct " % agents[agent]
    output = utils.ssh_exec(n, cmd)
    return output.rstrip('\n')

def get_pid(agent, n="localhost"): 
    if not is_agent_up(agent, n):
        return None
    cmd = "ps aux | egrep %s | grep -v egrep | grep -v defunct " % agents[agent]
    output = utils.ssh_exec(n, cmd).rstrip('\n')
    return int(output.split()[1])

def is_agent_up(a, n="localhost"):
    output = get_info(a, n)
    if len(output) == 0:
        return False
    else:
        return True
    
def is_up(nodes = ["localhost"]):
    for n in nodes:
        for a in agents:
            if not is_agent_up(a, n):
                return False
    return True

def is_any_up(nodes = ["localhost"]):
    up = []
    for n in nodes:
        for a in agents:
            if is_agent_up(a):
                up.add(a)
    return up

def kill_agent(agent, n = "localhost"):
    cmd = "kill -9 %s" % get_pid(agent, n)
    output = utils.ssh_exec(n, cmd)

def kill_all(nodes = ["localhost"]):
    for n in nodes:
        while is_agent_up("pm", n):
            kill_agent("pm", n)
        for a in agents:
            while is_agent_up(a, n):
                kill_agent(a, n)

def main():
    parser = OptionParser()
    parser.add_option("-K", "--kill-all", dest = "kill_all", action="store_true", help = "Kill FDS")
    parser.add_option("-A", "--is-any-up", dest = "is_any_up",  action="store_true", help = "Is any up")
    parser.add_option("-G", "--global-pid-table", dest = "global_pid_table", action="store_true", help = "Get PID table across nodes")
    parser.add_option("", "--fds-nodes", dest = "fds_nodes", default = "localhost",
                      help = "List of FDS nodes (for monitoring)")
    (options, args) = parser.parse_args()

    # FDS nodes
    options.nodes = {}
    for n in options.fds_nodes.split(","):
        options.nodes[n] = utils.get_ip(n)
    print "FDS nodes: ", options.nodes    
 
    if options.kill_all == True:
        print "Killing all"
        kill_all(options.nodes) 
    if options.is_any_up == True:
        print "Is any up:", is_any_up(options.nodes)
    if options.global_pid_table == True:
        print "global_pid_table:"
        print get_pid_table(options.nodes)

if __name__ == "__main__":
    main()
