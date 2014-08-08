#!/usr/bin/python
import os, re, sys, time
import numpy as np
import matplotlib.pyplot as plt

def add2dict(mydict, indexes, value):
    if len(indexes) == 1:
        if indexes[0] in mydict:
            mydict[indexes[0]].append(value)
        else:    
            mydict[indexes[0]] = [value]
        return mydict
    else:
        if indexes[0] in mydict:
            add2dict(mydict[indexes[0]], indexes[1:], value)
        else:    
            mydict[indexes[0]] = add2dict(dict(), indexes[1:], value) 
        return mydict

class Counters:
    def __init__(self):
        self.counters = {} # counter, node, volid
    def _add(self, node, agent, name, volid, type, value, tstamp):
        counts = self.counters
        counts = add2dict(counts, (name, node, agent, volid, type), (value, tstamp))
    def parse(self, data):
        tstamp = None
        for line in data.split('\n'):
            if tstamp is None:
                m = re.match(".*wrote: clock:(\d+\.\d+)", line)
                if m is not None:
                    tstamp = float(m.group(1))
            elif tstamp is not None:
                #am_put_dm.volume:6944500478244898626:volume=6944500478244898626
                m = re.match("([\w-]+)\.(\w+)\.perf\.(\w+)\.volume:\d+:volume=(\d+)\.(\w+)\s+(\d+)\s+(\d+)\s*", line)
                if m is not None:
                    node = m.group(1)
                    agent = m.group(2)
                    name =  m.group(3)
                    volid =  int(m.group(4))
                    cntr_type =  m.group(5)
                    value =  int(m.group(6))
                    tstamp =  int(m.group(7))
                    self._add(node, agent, name, volid, cntr_type, value, tstamp)
    def get_cntr(self, node, agent, name, cntr_type):
        series = {}
        for volid in self.counters[name][node][agent]:
            series[volid] = self.counters[name][node][agent][volid][cntr_type]
        for k in series:
            series[k] = zip(*series[k])
        return series

        ## for node, v1 in self.counters.items():
        ##     for agent, v2 in v1.items():  
        ##         for name, v3 in v2.items():
        ##                 for volid, v4 in v3.items():
        ##                     #print node, agent, name, volid, v4, v4.keys(), v4.get('count', None) #, v4[v4.keys()[v4.keys().index('count')]]
        ##                     #print node, agent, name, volid, v4.get(cntr_type, None)
        ##                     series.append(v4.get(cntr_type, None))
        return series


def get_rate(yarray, xarray):
    rate = np.arange(yarray.size)
    for i in range(1,len(yarray)):
        rate[i - 1] = ((yarray[i] - yarray[i - 1])/(xarray[i] - xarray[i - 1]))
    return rate

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

def plot_series_all_volumes(series, title = None, ylabel = None, xlabel = "Time [s]", latency = False, name = None):
    plt.figure()
    for v, s in series.iteritems():
        values = np.asarray(s[0])
        time = np.asarray(s[1])
        time -= time[0]
        if latency == False:
            rate = np.diff(values) / np.diff(time)
            time = time[:-1]
        else:
            rate = values / 1e6
        #print rate, rate.size, count.size
        plt.plot(time,rate)
    if title:
        plt.title(title)
    if ylabel:
        plt.ylabel(ylabel)
    if xlabel:
        plt.xlabel(xlabel)
    plt.savefig('%s.png' % (name))
    plt.show()

if __name__ == "__main__":
    c = Counters()
    f = open(sys.argv[1],"r")
    agent = sys.argv[2]
    counter = sys.argv[3]
    c.parse(f.read())
    f.close()
    #c.print_cntr("am_put_obj_req", "latency")
    node = "tie-fighter"
    # series = c.get_cntr(node, "AMAgent","am_put_obj_req", "count")
    # plot_series_all_volumes(series,"IOPS - PUT", "req/s", latency = False, name = "iops-put")
    # series = c.get_cntr(node, "AMAgent","am_get_obj_req", "count")
    # plot_series_all_volumes(series,"IOPS - GET", "req/s", latency = False, name  = "iops-get")
    # series = c.get_cntr(node, "AMAgent","am_put_obj_req", "latency")
    # plot_series_all_volumes(series,"Latency - PUT", "ns", latency = True, name = "latency-put")
    # series = c.get_cntr(node, "AMAgent","am_get_obj_req", "latency")
    # plot_series_all_volumes(series,"Latency - GET", "ms", latency = True, name = "latency-get")

    series = c.get_cntr(node, agent, counter, "count")
    plot_series_all_volumes(series,agent + " " + counter, "req/s", latency = False, name  = agent + "_" + counter)
    series = c.get_cntr(node, agent, counter, "latency")
    plot_series_all_volumes(series,agent + " " + counter, "ms", latency = True, name  = agent + "_" + counter)

