#!/usr/bin/python
import os,re,sys
from optparse import OptionParser
import numpy as np
import matplotlib.pyplot as plt
import time
from parse_cntrs import Counters
import dataset

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

NODES = {
        "node1" : "10.1.10.16",
        "node2" : "10.1.10.17",
        "node3" : "10.1.10.18",
        "tiefighter" : "10.1.10.102",
        "luke" : "10.1.10.222",
        "han" : "10.1.10.139",
        "chewie" : "10.1.10.80",
        "c3po" : "10.1.10.221",
    }

nodes = {}

def plot_series(series, title = None, ylabel = None, xlabel = "Time [ms]"):
    data = np.asarray(series)
    plt.plot(data)
    if title:
        plt.title(title)
    if ylabel:
        plt.ylabel(ylabel)
    if xlabel:
        plt.xlabel(xlabel)
    plt.show()

def get_fio(of):
    with open(of, "r") as f:
        rbw = 0.0
        riops = 0.0
        rclat = 0.0
        wbw = 0.0
        wiops = 0.0
        wclat = 0.0
        cntr = 0
        for l in f:
            tokens = l.rstrip("\n").split(";")
            rbw += float(tokens[6])
            riops += float(tokens[7])
            rclat += float(tokens[15])
            wbw += float(tokens[47])
            wiops += float(tokens[48])
            wclat += float(tokens[55])
            cntr += 1
    output = { 
                    "rbw" : rbw, 
                    "riops" : riops, 
                    "rclat" : rclat/cntr, 
                    "wbw" : wbw, 
                    "wiops" : wiops, 
                    "wclat" : wclat/cntr
                    }
    return output    
        
def get_avglat():
    outfiles = get_outfiles()
    lats = []
    for of in outfiles:        
        if options.ab_enable:
            cmd = "grep 'Time per request' %s |head -1| awk '{print $4}'" % (of)
            lat = float(os.popen(cmd).read().rstrip('\n'))
        elif options.block_enable:
            return None
        elif options.java_tester:
            cmd ="cat %s |grep Average| awk '{e+=$4; c+=1}END{print e/c*1e-6}'" % (of)
            lat = float(os.popen(cmd).read().rstrip('\n'))
        elif options.fio_enable:
            lat = (get_fio(of)["rclat"] + get_fio(of)["wclat"]) / 2
            lat = lat * 1e-3
        else:
            # cmd = "cat %s | grep Summary |awk '{print $17}'" % (of)
            cmd = "cat %s | grep Summary | awk '{e+=$17 ; i+=1}END{print e/i}'" % (of)
            lat = float(os.popen(cmd).read().rstrip('\n'))
        lats.append(lat)
    return sum(lats)/len(lats)


def get_outfiles():
    outfiles = ["%s/test.out" % (options.directory)]
    if not os.path.exists(outfiles[0]):
        outfiles.pop()
        i = 1
        candidate = "%s/test.out.%d" % (options.directory, i)
        while os.path.exists(candidate) == True: 
            outfiles.append(candidate)
            i += 1
            candidate = "%s/test.out.%d" % (options.directory, i)
    return outfiles


def get_avgth():
    outfiles = get_outfiles()
    ths = []
    for of in outfiles:        
        if options.ab_enable:
            cmd = "grep 'Requests per second' %s | awk '{print $4}'" % (of)
            th = float(os.popen(cmd).read().rstrip('\n'))
        elif options.block_enable:
            cmd = "cat %s | grep 'Experiment result' | awk '{print $11}'" % (of)
            th = float(os.popen(cmd).read().rstrip('\n'))
        elif options.java_tester:
            cmd = "cat %s | grep IOPs | awk '{e+=$2}END{print e}'" % (of)    
            th = float(os.popen(cmd).read().rstrip('\n'))
        elif options.fio_enable:
            th = (get_fio(of)["riops"] + get_fio(of)["wiops"]) / 2
        else:         
            # cmd = "cat %s | grep Summary |awk '{print $15}'" % (of)
            cmd = "cat %s | grep Summary |awk '{e+=$15}END{print e}'" % (of)
            th = float(os.popen(cmd).read().rstrip('\n'))

        ths.append(th)
    return sum(ths)/len(ths)    

def get_java_cntr():
    cmd = "cat %s| grep 'END serviceGetObject' | awk '{print $1}' | sed  -e 's/\[\|\]//g'" % (options.directory + "/java_counters.dat")
    output = os.popen(cmd).read()
    return [float(x) / 1e6 for x in filter(lambda x : len(x) > 0, output.split("\n"))]

def exist_exp():
    if os.path.exists("%s/test.out" % (options.directory)):
        return True
    else:
        return False

def compute_pidmap():
    file = open("%s/pidmap" % (options.directory),"r")
    for l in file:
        m = re.search("([\w-]+):(am|om|dm|sm|xdi)\s*:\s*(\d+)",l)
        if m is not None:
            node = m.group(1)
            agent = m.group(2)
            pid = int(m.group(3))
            if not node in g_pidmap:
                g_pidmap[node] = {}
            g_pidmap[node][agent] = pid
                
def convert_mb(x):
    if x[len(x) - 1] == "m":
        return float(x.rstrip('m')) * 1024
    elif x[len(x) - 1] == "g":
        return float(x.rstrip('g')) * 1024 * 1024
    else:
        return float(x)    

def get_mem(node, agent, pid):
    cmd = "egrep '^[ ]*%d' %s/%s/top/stdout | awk '{print $6}'" % (pid, options.directory, node)
    series = os.popen(cmd).read().split()
    return [convert_mb(x)/1024 for x in series]

def get_cpu(node, agent, pid):
    cmd = "egrep '^[ ]*%d' %s/%s/top/stdout | awk '{print $9}'" % (pid, options.directory, node)
    series = os.popen(cmd).read().split()
    return [float(x) for x in series]

def get_idle(node):
    cmd = "cat %s/%s/top/stdout |grep Cpu | awk '{print $8}'" % (options.directory, node)
    series = os.popen(cmd).read().split()
    return [float(x) for x in series]

def ma(v, w):
    _ma = []
    for i in range(w,len(v)):
        _ma.append(1.0*sum(v[i-w:i])/w)
    return _ma

def getmax(series, window = 5, skipbeg = 1, skipend = 1):
    _s = series[skipbeg:len(series)-skipend]
    if len(_s) == 0:
        return 0
    _ma = ma(_s, window)
    if len(_ma) == 0:
        return 0
    return max(_ma)

def getavg(series, window = 5, skipbeg = 2, skipend = 2):
    _s = series[skipbeg:len(series)-skipend]
    if len(_s) == 0:
        return 0
    _ma = ma(_s, window)
    if len(_ma) == 0:
        return 0
    return sum(_ma)/len(_ma)

#g_drives
def get_iops(node):
    iops_map = {}
    iops_series = []
    _file = open("%s/%s/ios/stdout" % (options.directory, node),"r")
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

def get_int(node):
    cmd = "cat %s/%s/col/stdout | awk '{print $12}' | tail -n +3" % (options.directory, node)
    values = [float(x.rstrip("\n")) for x in os.popen(cmd)]
    return values

def get_cs(node):
    cmd = "cat %s/%s/col/stdout | awk '{print $13}' | tail -n +3" % (options.directory, node)
    values = [float(x.rstrip("\n")) for x in os.popen(cmd)]
    return values

def get_await(node):
    await_map = {}
    await_series = []
    _file = open("%s/%s/ios/stdout" % (options.directory, node),"r")
    for l in _file:
        tokens = l.split()
        if len(tokens) >= 9:
            name = tokens[0]
            value = tokens[9]
            if name in g_drives:
                if name in await_map:
                    await_map[name].append(float(value))
                else:
                    await_map[name] = [float(value)]
    _file.close()
    all_values = []
    for n,values in await_map.items():
        all_values.append(values)
    #for e in zip(*all_values):
    #    await_series.append(sum(e)/len(e))
    await_series = [sum(e) for e in zip(*all_values)]
    return await_series


def get_test_parms():
    #results_2014-08-05_nvols\:1.threads\:1.type\:GET.nfiles\:10000.test_type\:tgen.fsize\:4096.nreqs\:100000.test
    m = re.match("(\w+)_(\d+-)_nvols:(\d+).threads:(\d+).type:(\w+).nfiles:(\d+).test_type:(\w+).fsize:(\d+).nreqs:(\d+).test", options.directory)

def print_counter(counters, agent, counter, ctype, table, node):
    c_node = node
    series = counters.get_cntr(c_node, agent, counter, ctype)
    if ctype == "count":
        if series is not None:
            for v in series:
                values = np.asarray(series[v][0])
                time = np.asarray(series[v][1])
                time -= time[0]
                rate = np.diff(values) / np.diff(time)
                value = getmax(rate)
                print agent + "-" + counter + ", ", value, ", ",
                table[agent+":"+counter+":count"] = value 
        else:
            print agent + "-" + counter + ", ", -1, ", ",
            table[agent+":"+counter+":count"] = -1 

    else:
        if series is not None:
            for v in series:
                value = getmax(series[v][0])/1e6
                print agent + "-" + counter + ", ", value, ", ",
                table[agent+":"+counter+":latency"] = value
        else:
            print agent + "-" + counter + ", ", -1, ", ",
            table[agent+":"+counter+":latency"] = -1

# results_2014-10-21_nvols:1.outstanding:50.test_type:tgen_java.fsize:4096.threads:4.nfiles:1000.nreqs:1000000.type:GET.test , results_2014-10-21_nvols:1.outstanding:50.test_type:tgen_java.fsize:4096.threads:4.nfiles:1000.nreqs:1000000.type:GET.test
def tokenize_name(name):
    _name = name.lstrip('results_').rstrip('\.test')
    m = re.match("^(\d+-\d+-\d+)_.*", _name)
    assert m != None
    date = m.group(1)
    _name = re.sub("[\d-]+_","",_name)
    tokens = _name.split('.')
    return tokens

# FIXME: multivolumes!

def main():
    parser = OptionParser()
    parser.add_option("-d", "--directory", dest = "directory", help = "Directory")
    parser.add_option("-n", "--name", dest = "name", help = "Name")
    parser.add_option("-N", "--node", dest = "node", help = "Node the counters have been collected")
    #parser.add_option("-c", "--column", dest = "column", help = "Column")
    parser.add_option("-p", "--plot-enable", dest = "plot_enable", action = "store_true", default = False, help = "Plot graphs")
    parser.add_option("-A", "--ab-enable", dest = "ab_enable", action = "store_true", default = False, help = "AB mode")
    parser.add_option("-B", "--block-enable", dest = "block_enable", action = "store_true", default = False, help = "Block mode")
    parser.add_option("-g", "--use-graphite-streaming", dest = "use_graphite_streaming", default = False, action = "store_true", help = "Use graphite streaming as input")
    parser.add_option("-j", "--java-tester", dest = "java_tester", default = False, action = "store_true", help = "Java tester output")
    parser.add_option("-b", "--dump-to-db", dest = "dump_to_db", default = None, help = "Dump to database as specified")
    parser.add_option("", "--fds-nodes", dest = "fds_nodes", default = "han",
                      help = "List of FDS nodes (for monitoring)")
    parser.add_option("-c", "--config-descr-tag", dest = "config_descr_tag", default = "amcache",
                      help = "Config description tag")
    parser.add_option("-F", "--fio-enable", dest = "fio_enable", action = "store_true", default = False, help = "FIO mode")
    global options
    (options, args) = parser.parse_args()
    compute_pidmap()
    options.nodes = {}
    for n in options.fds_nodes.split(","):
        nodes[n] = NODES[n]
    db = None 
    if options.dump_to_db != None:
        db = dataset.connect('sqlite:///%s' % options.dump_to_db)

    counters = Counters(graphite = False, volume_zero = True)
    c_file = open(options.directory + "/counters.dat")
    counters.parse(c_file.read())
    c_file.close()
    table = {}
    table["tag"] = options.config_descr_tag
    # counters.get_cntr()
    print options.name,",",
    table["name"] = options.name
    tokens = tokenize_name(options.name)
    for e in tokens:
        _t = e.split(':')
        table[_t[0]] = _t[1]
    time.sleep(1)
    th = get_avgth()
    print "th,", th,",",
    table["th"] = th
    lat = get_avglat()
    table["lat"] = lat
    print "lat,", get_avglat(),",",
    for node in nodes.keys():
        for agent,pid in g_pidmap[node].items():
            print "Agent,", agent, ",", node, ",",
            if options.plot_enable:
                plot_series(get_mem(node, agent, pid), "Memory - " + agent + " - " + options.name, "megabatyes")
                plot_series(get_cpu(node, agent, pid), "CPU - " + agent + " - " + options.name, "CPU utilization [%]")
            else:
                mem = getmax(get_mem(node, agent, pid))
                table[agent+":mem"] = mem
                print "maxmem,", mem,", ",
                cpu = getmax(get_cpu(node, agent, pid))
                print "maxcpu,", cpu,", ",
                table[agent+":cpu"] = cpu
        if options.plot_enable:
            plot_series(get_iops(node), "IOPS - " + options.name, "IOPS (disk)")
            plot_series(get_await(node), "AWAIT - " + options.name, "AWAIT (disk)")
            plot_series(get_idle(node), "Avg Idle - " + options.name, "Idle CPU Time [%]")
        else:
            iops = getmax(get_iops(node))
            print "maxiops,", iops,", ",
            table["iops"] = iops
            await = getmax(get_await(node))
            print "maxawait,", await,", ",
            table["await"] = await
            idle = getavg(get_idle(node))
            print "avgidle,",idle,", ",
            table["idle"] = idle
            interrupts = getavg(get_int(node))
            print "interrupts,",interrupts,", ",
            table["interrupts"] = interrupts
            cs = getavg(get_cs(node))
            print "cs,",cs,", ",
            table["cs"] = cs
    print_counter(counters, "am", "am_get_obj_req", "latency", table, options.node)
    print_counter(counters, "am", "am_get_obj_req", "count", table, options.node)
    # am:am_get_obj_req,am:am_get_sm,am:am_get_dm,am:am_get_qos
    print_counter(counters, "am", "am_get_sm", "latency", table, options.node)
    print_counter(counters, "am", "am_get_dm", "latency", table, options.node)
    print_counter(counters, "am", "am_get_qos", "latency", table, options.node)


    print_counter(counters, "am", "am_put_obj_req", "latency", table, options.node)
    print_counter(counters, "am", "am_put_obj_req", "count", table, options.node)
    print_counter(counters, "am", "am_put_sm", "latency", table, options.node)
    print_counter(counters, "am", "am_put_dm", "latency", table, options.node)
    print_counter(counters, "am", "am_put_qos", "latency", table, options.node)
    
    javalat = getavg(get_java_cntr())
    print "avgjavalat,", javalat, ", ",
    table["javalat"] = javalat
    # print_counter(counters, "am","am_stat_blob_obj_req", "latency")
    # print_counter(counters, "sm","get_obj_req", "latency")
    #print_counter("AMAgent","am_stat_blob_obj_req", "latency")

    # series = counters.get_cntr(c_node,"AMAgent","am_get_obj_req", "latency")
    # if series is not None:
    #     for v in series:
    #         print "am_get_latency: ", getmax(series[v][0])/1e6,
    # else:
    #     print "am_get_latency: ", -1,
    # series = counters.get_cntr(c_node,"AMAgent","am_stat_blob_obj_req", "latency")
    # if series is not None:
    #     for v in series:
    #         print "am_stat_blob_latency: ", getmax(series[v][0])/1e6,
    # else:
    #     print "am_get_latency: ", -1,
    print "" 
    if options.dump_to_db != None and db != None:
        db["experiments"].insert(table)
        # for e in db["experiments"].all():
        #    print e

if __name__ == "__main__":
    main()
