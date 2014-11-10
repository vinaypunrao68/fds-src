#!/usr/bin/python
import os, re, sys
from optparse import OptionParser

agents={
    "am" : "bare_am", 
    "xdi" : "com\.formationds\.am", 
    "om" : "com\.formationds\.om", 
    "sm" : "StorMgr", 
    "dm" : "DataMgr", 
    "pm" : "platformd"
}

def get_info(agent): 
    cmd = "ps aux | egrep %s | grep -v egrep " % agents[agent]
    return os.popen(cmd).read().rstrip('\n')

def get_pid(agent): 
    if not is_agent_up(agent):
        return None
    cmd = "ps aux | egrep %s | grep -v egrep " % agents[agent]
    output = os.popen(cmd).read().rstrip('\n')
    return int(output.split()[1])

def get_pid_table():
    if not is_up():
        return {}
    table = {}
    for a in agents:
        table[a] = get_pid(a)
    return table

def is_agent_up(a):
    output = get_info(a)
    if len(output) == 0:
        return False
    else:
        return True
    
def is_up():
    for a in agents:
        if not is_agent_up(a):
            return False
    return True

def is_any_up():
    up = []
    for a in agents:
        if is_agent_up(a):
            up.add(a)
    return up

def get_info_services():
    for a in agents:
        print s,"-->\n",get_info(a)

def kill_agent(agent):
    cmd = "kill -9 %s" % get_pid(agent)
    os.system(cmd)

def kill_all():
    for a in agents:
        kill_agent(a)

def main():
    parser = OptionParser()
    parser.add_option("-U", "--is-up", dest = "is_up", action="store_true", help = "Test is the system is up")
    parser.add_option("-u", "--is-agent--up", dest = "is_agent_up", help = "Test if an agent is up")
    parser.add_option("-P", "--pid-table", dest = "pid_table", action="store_true", help = "Get PID table")
    parser.add_option("-p", "--agent-pid", dest = "agent_pid", help = "Get PID for an agent")
    parser.add_option("-K", "--kill-all", dest = "kill_all", action="store_true", help = "Kill FDS")
    parser.add_option("-k", "--kill-agent", dest = "kill_agent", help = "Kill agent")
    parser.add_option("-A", "--is-any-up", dest = "is_any_up",  action="store_true", help = "Is any up")
    (options, args) = parser.parse_args()

    if options.is_up == True:
        print "FDS is up:", is_up()
    if options.is_agent_up != None:
        print "Agent", options.is_agent_up, "is up", is_agent_up(options.is_agent_up) 
    if options.pid_table == True:
        print "PID table:", get_pid_table()
    if options.agent_pid != None:
        print "Agent", options.agent_pid, "is up", get_pid(options.agent_pid)
    if options.kill_all == True:
        print "Killing all"
        kill_all() 
    if options.kill_agent != None:
        print "Kill agent", options.kill_agent
        kill_agent(options.kill_agent)
    if options.is_any_up == True:
        print "Is any up:", is_any_up()

if __name__ == "__main__":
    main()
