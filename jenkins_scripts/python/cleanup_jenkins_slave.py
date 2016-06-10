#!/usr/bin/python

import os
import sys
import psutil
import re

# List processes here, each process must be a list, one element per item,
# of the full command & arguments - similar to what you would pass into
# something like subprocess.Popen.

preserve_process_list = [
        'kworker',
        'getty',
        'acpi_thermal_pm',
        'ata_sff',
        'bioset',
        'charger_manager',
        'cleanup_jenkins',
        'cron',
        'crypto',
        'dbus-daemon',
        'deferwq',
        'devfreq_wq',
        'ecryptfs-kthrea',
        'ext4-rsv-conver',
        'fsnotify_mark',
        'ib_addr',
        'ib_cm',
        'ib_mcast',
        'init',
        'ipv6_addrconf',
        'irqbalance',
        'iscsi_eh',
        'iw_cm_wq',
        'jbd2/sda1-8',
        'jbd2/sda2-8',
        'jfsIO',
        'jfsCommit',
        'jfsSync',
        'kauditd',
        'kblockd',
        'kdevtmpfs',
        'khelper',
        'khugepaged',
        'khungtaskd',
        'kintegrityd',
        'kpsmoused',
        'ksmd',
        'ksoftirqd',
        'kswapd0',
        'kthreadd',
        'kthrotld',
        'kvm-irqfd-clean',
        'login',
        'md',
        'migration',
        'netns',
        'ntpd',
        'nfsiod',
        'perf',
        'rcu_bh',
        'rcuob',
        'rcuos',
        'rcu_sched',
        'rdma_cm',
        'rpcbind',
        'rpc.idmapd',
        'rpciod',
        'rpc.statd',
        'rsyslogd',
        'scsi_eh',
        'scsi_tmf',
        'systemd-logind',
        'systemd-udevd',
        'ttm_swap',
        'upstart-file-bridge',
        'upstart-socket-bridge',
        'upstart-udev-bridge',
        'vballoon',
        'vmstat',
        'watchdog',
        'writeback',
        'xfsalloc',
        'xfs_mru_cache'
]

whitelist = [
        ['/bin/sh', '-c', '/usr/sbin/sshd -D -o UsePAM=no'],
        ['/usr/sbin/sshd', '-D', '-o', 'UsePAM=no'],
        ['bash', '-c', 'cd "/home/jenkins" && java -Dorg.jenkinsci.plugins.gitclient.GitClient.quietRemoteBranches=true -jar slave.jar'],
        ['java', '-Dorg.jenkinsci.plugins.gitclient.GitClient.quietRemoteBranches=true', '-jar', 'slave.jar'],
        ['sshd: root@notty'],
        ['sudo', 'su', '-'],
        ['su', '-'],
        ['/sbin/rpcbind', '-w'],
        ['java', '-jar', 'slave.jar'],
        ['/usr/bin/python', 'jenkins_scripts/python/cleanup_jenkins_slave.py'],
        ['/bin/bash', '-l', 'jenkins_scripts/jenkins_build_test_coroner_cleanup.sh'],
        ['/bin/bash', '-l', 'jenkins_scripts/long_system_test.sh'],
        ['/bin/bash', '-l', 'jenkins_scripts/jenkins_build_test_coroner_cleanup.sh', 'compile_only'],
        ['/bin/bash', '-l', 'jenkins_scripts/jenkins_build_test_coroner_cleanup.sh', 'jenkins_build_aborted'],
        ['/bin/bash', '-l', 'jenkins_scripts/jenkins_build_test_coroner_cleanup.sh', 'jenkins_build_on_master_commit'],
        ['/bin/bash', '-lx', 'jenkins_scripts/jenkins_build_test_coroner_cleanup.sh'],
        ['/bin/bash', '-lx', 'jenkins_scripts/long_system_test.sh'],
        ['/bin/bash', '-lx', 'jenkins_scripts/jenkins_build_test_coroner_cleanup.sh', 'jenkins_build_aborted'],
        ['/bin/bash', '-lx', 'jenkins_scripts/jenkins_build_test_coroner_cleanup.sh', 'jenkins_build_on_master_commit'],
        ['/bin/bash', '-lx', 'jenkins_scripts/jenkins_build_test_coroner_cleanup.sh', 'compile_only'],
        ['/bin/bash', '-lxe', 'jenkins_scripts/jenkins_build_test_coroner_cleanup.sh'],
        ['/bin/bash', '-lxe', 'jenkins_scripts/long_system_test.sh'],
        ['/bin/bash', '-lxe', 'jenkins_scripts/jenkins_build_test_coroner_cleanup.sh', 'jenkins_build_aborted'],
        ['/bin/bash', '-lxe', 'jenkins_scripts/jenkins_build_test_coroner_cleanup.sh', 'jenkins_build_on_master_commit'],
        ['/bin/bash', '-lxe', 'jenkins_scripts/jenkins_build_test_coroner_cleanup.sh', 'compile_only'],
        ['dhclient', '-1', '-v', '-pf', '/run/dhclient.eth0.pid', '-lf', '/var/lib/dhcp/dhclient.eth0.leases', 'eth0'],
        # children of daily cron job
        ['/bin/sh', '-c', 'test -x /usr/sbin/anacron || ( cd / && run-parts --report /etc/cron.daily )'],
        ['run-parts', '--report', '/etc/cron.daily'],
        ['/bin/bash', '/etc/cron.daily/mlocate'],
        ['flock', '--nonblock', '/run/mlocate.daily.lock', '/usr/bin/ionice', '-c3', '/usr/bin/updatedb.mlocate'],
        ['/usr/bin/updatedb.mlocate'],
        # nfs server
        ['nfsv4.0-svc']]

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
        print "OK (cmd): {} {} {}".format(proc.name(), proc.pid, cmd)
    elif proc.name() in global_whitelist:
        print "OK (whl): {} {} {}".format(proc.name(), proc.pid, cmd)
    # hardware_long_system_test runs with parameter as list of tests which can be changed for every run so ignore this case
    elif proc.name().startswith('hardware_long'):
        print "OK (rls): {}".format(proc.name(), proc.pid, cmd)
    else:
        for save_proc in preserve_process_list:
            if proc.name().find(save_proc) != -1:
                print "OK (ppl): {}".format(proc.name(), proc.pid, cmd)
                break
        else:
            print "KILL: {} {} {}".format(proc.name(), proc.pid, cmd)
            killed.append(proc.name())
            proc.kill()

if len(killed) > 0:
    print "Processes killed:"
    for process in killed:
        print process
    sys.exit(1)
else:
    print "No processes killed"
    sys.exit(0)
