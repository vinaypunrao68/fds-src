#!/usr/bin/python

import os
import sys
import psutil

# List processes here, each process must be a list, one element per item,
# of the full command & arguments - similar to what you would pass into
# something like subprocess.Popen.
whitelist = [
        ['/bin/sh', '-c', '/usr/sbin/sshd -D -o UsePAM=no'],
        ['/usr/sbin/sshd', '-D', '-o', 'UsePAM=no'],
        ['bash', '-c', 'cd "/home/jenkins" && java  -jar slave.jar'],
        ['java', '-jar', 'slave.jar'],
        ['sshd: root@notty'],
        ['/usr/bin/python', '/tmp/clean_jenkins.py'],
        ['sudo', 'su', '-'],
        ['su', '-']
]

# Be very careful adding stuff here - if you add 'java' you will
# not kill off any java process
global_whitelist = [
        "/bin/bash",
        "bash",
        "sshd",
        "ssh",
        "zsh",
        "tmux",
        "python"
]

killed = []

sys.stdout.flush()

for proc in psutil.process_iter():
    cmd = psutil.Process(pid=proc.pid).cmdline()
    print "REPORT: {} {} {}".format(proc.name(), proc.pid, cmd)

for proc in psutil.process_iter():
    cmd = psutil.Process(pid=proc.pid).cmdline()
    if cmd in whitelist:
        print "OK: {} {} {}".format(proc.name(), proc.pid, cmd)
    elif proc.name() in global_whitelist:
        print "OK: {} {} {}".format(proc.name(), proc.pid, cmd)
    else:
        print "KILL: {} {} {}".format(proc.name(), proc.pid, cmd)
        killed.append(cmd)
        proc.kill()

if len(killed) > 0:
    print "Processes killed:"
    for process in killed:
        print process
    sys.exit(1)
else:
    print "No processes killed"
    sys.exit(0)
