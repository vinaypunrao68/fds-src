#!/usr/bin/python

import os
import sys
import psutil
import re

# List processes here, each process must be a list, one element per item,
# of the full command & arguments - similar to what you would pass into
# something like subprocess.Popen.
whitelist = [
        ['/bin/sh', '-c', '/usr/sbin/sshd -D -o UsePAM=no'],
        ['/usr/sbin/sshd', '-D', '-o', 'UsePAM=no'],
        ['bash', '-c', 'cd "/home/jenkins" && java -Dorg.jenkinsci.plugins.gitclient.GitClient.quietRemoteBranches=true -jar slave.jar'],
        ['java', '-Dorg.jenkinsci.plugins.gitclient.GitClient.quietRemoteBranches=true', '-jar', 'slave.jar'],
        ['sshd: root@notty'],
        ['sudo', 'su', '-'],
        ['su', '-'],
        ['/usr/bin/python', 'jenkins_scripts/python/cleanup_jenkins_slave.py'],
        ['/bin/bash', '-l', 'jenkins_scripts/jenkins_build_test_coroner_cleanup.sh'],
        ['/bin/bash', '-l', 'jenkins_scripts/long_system_test.sh'],
        ['/bin/bash', '-l', 'jenkins_scripts/jenkins_build_test_coroner_cleanup.sh', 'jenkins_build_aborted'],
        ['/bin/bash', '-l', 'jenkins_scripts/jenkins_build_test_coroner_cleanup.sh', 'jenkins_build_on_master_commit'],
        ['/bin/bash', '-l', 'jenkins_scripts/aws_long_system_test.sh']
]

# Be very careful adding stuff here - if you add 'java' you will
# not kill off any java process
global_whitelist = [
        "bash",
        "sshd",
        "ssh",
        "zsh",
        "tmux",
        "python"
]

jenkins_build_regex = re.compile ('/tmp/hudson[0-9]*\.sh')

killed = []

sys.stdout.flush()

for proc in psutil.process_iter():
    cmd = psutil.Process(pid=proc.pid).cmdline()

    if len (cmd) > 2:
        if cmd[0] == '/bin/bash' and cmd[1] == '-le':
            matches = re.match (jenkins_build_regex, cmd[2])

            if matches is not None:
                print "OK (re match): {} {} {}".format(proc.name(), proc.pid, cmd)
            else:
                print "KILL (no re match): {} {} {}".format(proc.name(), proc.pid, cmd)
                killed.append(cmd)
                proc.kill()
            continue

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
