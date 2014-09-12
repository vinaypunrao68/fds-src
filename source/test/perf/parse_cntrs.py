#!/usr/bin/python
import os, re, sys, time
import numpy as np
import matplotlib.pyplot as plt
from optparse import OptionParser
from operator import itemgetter

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
    def __init__(self, graphite):
        self.use_graphite = graphite
        self.counters = {} # counter, node, volid

    def _add(self, node, agent, name, volid, type, value, tstamp):
        add2dict(self.counters, (name, node, agent, volid, type), (value, tstamp))

    def parse(self, data):
        tstamp = None
        for line in data.split('\n'):
            # if tstamp is None:
            #     m = re.match(".*wrote: clock:(\d+\.\d+)", line)
            #     if m is not None:
            #         tstamp = float(m.group(1))
#             elif tstamp is not None:
                if self.use_graphite == True:
                    #am_put_dm.volume:6944500478244898626:volume=6944500478244898626
                    m = re.match("([\w-]+)\.(\w+)\.perf\.(\w+)\.volume:\d+:volume=(\d+)\.(\w+)\s+(\d+)\s+(\d+)\s*", line)
                else:
                    # tiefighter am am_put_dm.volume:3125441350624440090:volume=3125441350624440090.latency 2720579 1408730115
                    m = re.match("([\w-]+)\s+(\w+)\s+(\w+)\.volume:\d+:volume=(\d+)\.(\w+)\s+(\d+)\s+(\d+)\s*", line)
                if m is not None:
                    node = m.group(1)
                    agent = m.group(2)
                    name =  m.group(3)
                    volid =  int(m.group(4))
                    cntr_type =  m.group(5)
                    value =  int(m.group(6))
                    tstamp =  int(m.group(7))
                    if volid != 0:
                        self._add(node, agent, name, volid, cntr_type, value, tstamp)
                    else:
                        print "Warning: volid is zero for ",node, agent, name, volid, cntr_type, value, tstamp
                        # self._add(node, agent, name, volid, cntr_type, value, tstamp)

    def get_cntr(self, node, agent, name, cntr_type):
        series = {}
        try:
            for volid in self.counters[name][node][agent]:
                    series[volid] = self.counters[name][node][agent][volid][cntr_type]
        except KeyError:
            return {}
        for k in series:
            series[k] = zip(*series[k])
        return series

    def get_cntr_types(self):
        return self.counters.keys()

    def get_nodes(self):
        nodes = set()
        types = self.get_cntr_types()
        for t in types:
            # print self.counters[t].keys()
            # [nodes.add(x) for x in self.counters[t].keys() if not x in options.exclude_nodes]
            [nodes.add(x) for x in self.counters[t].keys()]
        return list(nodes)

    def get_agents(self):
        agents = set()
        types = self.get_cntr_types()
        nodes = self.get_nodes()
        for t in types:
            for n in nodes:
                if n in self.counters[t]:
                    [agents.add(x) for x in self.counters[t][n].keys()]
        return list(agents)

    def get_volumes(self):
        volumes = set()
        types = self.get_cntr_types()
        nodes = self.get_nodes()
        agents = self.get_agents()
        for t in types:
            for n in nodes:
                if n in self.counters[t]:
                    for a in agents:
                        if a in self.counters[t][n]:
                            [volumes.add(x) for x in self.counters[t][n][a].keys()]
        return list(volumes)

    def get_names(self):
        names = []
        for t, second1 in self.counters.iteritems():
                for n, second2 in second1.iteritems():
                    for a, second3 in second2.iteritems():
                        for v, second4 in second3.iteritems():
                            for cntr_t, second5 in second4.iteritems():
                                names.append(str(t) + ":" + str(n) + ":" + str(a) + ":" + str(v) + ":" + str(cntr_t)
                                         + ":" + str(len(second5)) + "-" + str(len(second5)))
        return names

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

def new_figure(name):
    fig = plt.figure(name)

def plot_series_all_volumes(series, title = None, ylabel = None, xlabel = "Time [s]", latency = False, name = None, figname = None):
    plt.figure(figname, figsize=(3, 3))
    fig = plt.gcf()
    # fig.set_size_inches(10,50)
    ax = plt.subplot(111)
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
        plt.plot(time, rate, label = name + ":" + str(v)[:4])
    if title:
        plt.title(title)
    if ylabel:
        plt.ylabel(ylabel)
    if xlabel:
        plt.xlabel(xlabel)
    plt.ylim(ymin = 0)
    box = ax.get_position()
    #ax.set_position([box.x0, box.y0, box.width * 0.8, box.height])
    ax.set_position([0.1, 0.1, 0.5, 0.8])
    ax.legend(loc='center left', bbox_to_anchor=(1, 0.5))	
    ax.set_aspect("auto")
    #plt.legend(loc = "lower right")

def plot_show_and_save(name):
    # plt.legend()
    plt.savefig('%s.png' % (name))
    plt.show()

# TODO: options
# TODO: list all parsed counters
# TODO: plot multiple
# TODO: plot real time

#  {vid : [(v1, v2,...), (t1, t2,...)], ...}

def sum_series(series1, series2):
    series = {}
    for k in series1.keys():
        series[k] = None
    for k in series2.keys():
        series[k] = None
    for v in series:
        pair1 = None
        pair2 = None
        if v in series1:
            pair1 = series1[v]
        if v in series2:
            pair2 = series2[v]
        if pair1 != None and pair2 != None:
            vector = zip(*pair1) + zip(*pair2)
        elif pair1 != None:
            vector = zip(*pair1)
        elif pair2 != None:
            vector = zip(*pair2)
        else:
            continue
        # assert pair1 is not None and pair2 is not None
        # l = len(pair1[0])
        # assert len(pair1[0]) == len(pair2[0])
        # assert len(pair1[1]) == len(pair2[1])
        # assert len(pair1[0]) == len(pair1[1])
        # assert len(pair2[0]) == len(pair2[1])
        # for j in range(l):
        #     assert pair1[1][j] == pair2[1][j]

        # need to associate timestamp - value, then order them, compact them and rezipthem

        # sort vector by timestamp
        sorted_vector = sorted(vector, key=itemgetter(1))
        # compact vector
        compacted_vector = []
        for e in sorted_vector:
            if len(compacted_vector) > 0:
                if compacted_vector[-1][1] == e[1]: # same timestamp
                    val = compacted_vector[-1][0] + e[0]
                    compacted_vector[-1] = val, compacted_vector[-1][1] # accumulate value
                else:
                    compacted_vector.append(e)
            else:
                compacted_vector.append(e)
        series[v] = zip(*compacted_vector)

        # series[v][1] = pair1[1]
        # series[v][0] = [pair1[0][j] + pair2[0][j] for j in range(l)]
    return series

def plot_qos_graphs(cntrs, node, agent):
# FIXME: debug counters
# fixme compose latency
    user_level_cntrs = ["am_get_obj_req",
        "am_put_obj_req",
        "am_delete_obj_req",
        ]
    fds_level_cntrs = user_level_cntrs + ["am_stat_blob_obj_req",
        "am_set_blob_meta_obj_req",
        "am_get_volume_meta_obj_req",
        "am_get_blob_meta_obj_req",
        "am_start_blob_obj_req",
        "am_commit_blob_obj_req",
        "am_abort_blob_obj_req",
        ]
    test = ["am_stat_blob_obj_req", "am_get_obj_req", "am_get_qos"]
    # {vid : [(v1, v2,...), (t1, t2,...)], ...}
    # print cntrs.get_cntr(node, agent, "am_get_qos", "count")

    test_cntr = [cntrs.get_cntr(node, agent, x, "count") for x in fds_level_cntrs]
    series = reduce(sum_series, test_cntr)

    new_figure("Throughput Aggregate")
    new_figure("Latency")
    #
    # series = counters.get_cntr(node, agent, counter, "count")
    plot_series_all_volumes(series,"Throughput", "req/s", latency = False,
                            name = "", figname = "Throughput Aggregate")
    # series = counters.get_cntr(node, agent, counter, "latency")
    # plot_series_all_volumes(series,"Latency", "ms", latency = True, name = agent + "_" + counter, figname = "Latency")
    # plot_show_and_save("somename")


def main():
    parser = OptionParser()
    parser.add_option("-f", "--file", dest = "filein", help = "Input file")
    parser.add_option("-c", "--counters", dest = "counters", help = "List of agent:counter,...")
    parser.add_option("-n", "--node", dest = "node", help = "Node")
    parser.add_option("-s", "--search-counter", dest = "search_counter", default = None, help = "Search counter regexp")
    parser.add_option("-g", "--use-graphite-streaming", dest = "use_graphite_streaming", default = False, action = "store_true", help = "Use graphite streaming as input")

    #parser.add_option("-c", "--column", dest = "column", help = "Column")
    global options

    (options, args) = parser.parse_args()


    options.exclude_nodes = []

    # FIXME: revisit Counters interface
    c = Counters(options.use_graphite_streaming)
    f = open(options.filein, "r")
    c.parse(f.read())
    f.close()
    # print c.counters
    types = c.get_cntr_types()
    print "Counter types:", types
    nodes = c.get_nodes()
    print "Nodes:", nodes
    agents = c.get_agents()
    print "Agents:", agents
    volumes = c.get_volumes()
    print "Volumes:", volumes

    names = c.get_names()
    if options.search_counter != None:
        for t in names:
            m = re.search(options.search_counter, t)
            if m is not None:
                print t
        os._exit(0)

    assert options.node in c.get_nodes(), "Node does not exist " + options.node

    #a = { 11 : [(11,22,33,11),(1,2,4,5)], 22 : [(11,22,33,44),(1,2,3,4)]}
    #b = { 11 : [(11,22,33,11),(1,2,3,6)], 32 : [(11,22,33,44),(1,2,3,4)]}
    #print a, b
    #print sum_series(a, b)

    # plot_qos_graphs(c, "tiefighter", "am")

    for name in options.counters.split(","):
        agent, counter = name.split(":")
        assert agent in c.get_agents(), "Agent does not exist " + agent
        assert counter in c.get_cntr_types(), "Counter does not exist " + counter
        #c.print_cntr("am_put_obj_req", "latency")
        node = options.node
        # series = c.get_cntr(node, "AMAgent","am_put_obj_req", "count")
        # plot_series_all_volumes(series,"IOPS - PUT", "req/s", latency = False, name = "iops-put")
        # series = c.get_cntr(node, "AMAgent","am_get_obj_req", "count")
        # plot_series_all_volumes(series,"IOPS - GET", "req/s", latency = False, name  = "iops-get")
        # series = c.get_cntr(node, "AMAgent","am_put_obj_req", "latency")
        # plot_series_all_volumes(series,"Latency - PUT", "ns", latency = True, name = "latency-put")
        # series = c.get_cntr(node, "AMAgent","am_get_obj_req", "latency")
        # plot_series_all_volumes(series,"Latency - GET", "ms", latency = True, name = "latency-get")
        new_figure("Throughput")
        new_figure("Latency")
        series = c.get_cntr(node, agent, counter, "count")
        plot_series_all_volumes(series,"Throughput", "req/s", latency = False, name = agent + "_" + counter, figname = "Throughput")
        series = c.get_cntr(node, agent, counter, "latency")
        plot_series_all_volumes(series,"Latency", "ms", latency = True, name = agent + "_" + counter, figname = "Latency")
    plot_show_and_save("somename") # FIXME: name


if __name__ == "__main__":
    main()
