#!/usr/bin/python
import os,re,sys
from optparse import OptionParser
import numpy as np
import matplotlib.pyplot as plt
import time

g_pidmap = {}

g_drives = [ "sdg ",
"sdc",
"sdb", 
"sdd", 
"sde", 
"sdh", 
"sda", 
"sdf", 
"nb0", 
"nb16" ]


def plot_series(options, series, title = None, ylabel = None, xlabel = "Time [ms]"):
    data = np.asarray(series)
    plt.plot(data)
    if title:
        plt.title(title)
    if ylabel:
        plt.ylabel(ylabel)
    if xlabel:
        plt.xlabel(xlabel)
    plt.show()

def get_avglat(options):
    if options.ab_enable:
        cmd = "grep 'Time per request' %s/out.%s |head -1| awk '{print $4}'" % (options.directory, options.name)
    else:
        cmd = "cat %s/out.%s | grep Summary |awk '{print $15}'" % (options.directory, options.name)
    return float(os.popen(cmd).read().rstrip('\n'))

def get_avgth(options):
    if options.ab_enable:
        cmd = "grep 'Requests per second' %s/out.%s | awk '{print $4}'" % (options.directory, options.name)
    else:        
        cmd = "cat %s/out.%s | grep Summary |awk '{print $13}'" % (options.directory, options.name)
    return float(os.popen(cmd).read().rstrip('\n'))

def exist_exp(options):
    if os.path.exists("%s/map.%s" % (options.directory, options.name)):
        return True
    else:
        return False

def compute_pidmap(options):
    file = open("%s/map.%s" % (options.directory, options.name),"r")
    for l in file:
        m = re.search("Map: (am|om|dm|sm|) --> (\d+)",l)
        if m is not None:
            agent = m.group(1)
            pid = int(m.group(2))
            g_pidmap[agent] = pid
                
def convert_mb(x):
    if x[len(x) - 1] == "m":
        return float(x.rstrip('m')) * 1024
    elif x[len(x) - 1] == "g":
        return float(x.rstrip('g')) * 1024 * 1024
    else:
        return float(x)    

def get_mem(options, agent, pid):
    cmd = "egrep '^[ ]*%d' %s/top.%s | awk '{print $6}'" % (pid, options.directory, options.name)
    series = os.popen(cmd).read().split()
    return [convert_mb(x)/1024 for x in series]

def get_cpu(options, agent, pid):
    cmd = "egrep '^[ ]*%d' %s/top.%s | awk '{print $9}'" % (pid, options.directory, options.name)
    series = os.popen(cmd).read().split()
    return [float(x) for x in series]

def get_idle(options):
    cmd = "cat %s/top.%s |grep Cpu | awk '{print $8}'" % (options.directory, options.name)
    series = os.popen(cmd).read().split()
    return [float(x) for x in series]

def ma(v, w):
    _ma = []
    for i in range(w,len(v)):
        _ma.append(1.0*sum(v[i-w:i])/w)
    return _ma

def getmax(series, window = 5, skipbeg = 2, skipend = 2):
    _s = series[skipbeg:len(series)-skipend]
    if len(_s) == 0:
        return None
    _ma = ma(_s, window)
    if len(_ma) == 0:
        return None
    return max(_ma)

def getavg(series, window = 5, skipbeg = 2, skipend = 2):
    _s = series[skipbeg:len(series)-skipend]
    if len(_s) == 0:
        return None
    _ma = ma(_s, window)
    if len(_ma) == 0:
        return None
    return sum(_ma)/len(_ma)

#g_drives
def get_iops(options): 
    iops_map = {}
    iops_series = []
    _file = open("%s/iostat.%s" % (options.directory, options.name),"r")
    for l in _file:
        tokens = l.split()
        if len(tokens) >= 2:
            name = tokens[0]
            value = tokens[1]
            if name in g_drives:
                if name in iops_map:
                    iops_map[name].append(float(value))
                else:
                    iops_map[name] = [float(value)]
    _file.close()
    all_values = []
    for n,values in iops_map.items():
        all_values.append(values)
    #for e in zip(*all_values):
    #    iops_series.append(sum(e)/len(e))
    iops_series = [sum(e) for e in zip(*all_values)]
    return iops_series   

def main(options):
    compute_pidmap(options)
    print options.name,",",
    time.sleep(1)
    print "th,", get_avgth(options),",",
    print "lat,", get_avglat(options),",",
    for agent,pid in g_pidmap.items():
        print "Agent,",agent,",",
        if options.plot_enable:
            plot_series(options, get_mem(options,agent, pid), "Memory - " + agent + " - " + options.name, "megabatyes")
            plot_series(options, get_cpu(options,agent, pid), "CPU - " + agent + " - " + options.name, "CPU utilization [%]")
        else:
            print "maxmem,",getmax(get_mem(options,agent, pid)),",",
            print "maxcpu,",getmax(get_cpu(options,agent, pid)),",",
    if options.plot_enable:
        plot_series(options, get_iops(options), "IOPS - " + options.name, "IOPS (disk)")
        plot_series(options, get_idle(options), "Avg Idle - " + options.name, "Idle CPU Time [%]")
    else:
        print "maxiops,",getmax(get_iops(options)),",",
        print "avgidle,",getavg(get_idle(options)),",",
    print "" 

if __name__ == "__main__":
    parser = OptionParser()
    parser.add_option("-d", "--directory", dest = "directory", help = "Directory")
    parser.add_option("-n", "--name", dest = "name", help = "Name")
    #parser.add_option("-c", "--column", dest = "column", help = "Column")
    parser.add_option("-p", "--plot-enable", dest = "plot_enable", action = "store_true", default = False, help = "Plot graphs")
    parser.add_option("-A", "--ab-enable", dest = "ab_enable", action = "store_true", default = False, help = "AB mode")
    (options, args) = parser.parse_args()
    main(options)
