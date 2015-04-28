#!/usr/bin/python

import os, re, sys
import datetime
import subprocess

import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

# compute duration of migration
prefix=sys.argv[1]

with open(prefix + "/migration.start", "r") as _f:
    _s = _f.read().rstrip("\n")
    start=datetime.datetime.strptime(_s, "%a %b %d %H:%M:%S %Z %Y")

with open(prefix + "/migration.end", "r") as _f:
    _s = _f.read().rstrip("\n")
    end=datetime.datetime.strptime(_s, "%a %b %d %H:%M:%S %Z %Y")

duration = (end - start).total_seconds()

cmd = "grep StorMgr %s | awk '{print $9}'" % (prefix + "/monitor.top")
cpus = [float(x) for x in filter(lambda x : len(x)> 0, subprocess.check_output(cmd, shell=True).split('\n'))]
#cpus = filter(lambda x : x > 5.0, cpus)
norm_cpu =  sum(cpus)/duration
print prefix, duration, norm_cpu
with open("cpus.csv", "a") as _f:
    _f.write(prefix + ' ')
    [_f.write(str(x) + ',') for x in cpus]
    _f.write('\n')

#plt.plot(cpus)
#plt.savefig("fig.png")

