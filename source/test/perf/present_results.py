#!/usr/bin/python

import os,re,sys
import tabulate
import dataset
import operator 
from influxdb import client as influxdb
import numpy
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import traceback
import math

agents = ["am", "xdi", "sm", "dm", "sm", "om"]

test_types = {"s3" : "S3 tester",
                "s3_java" : "S3 java tester",
                "fio" : "Block (FIO)"
                }

mixes = { "get" : "100% GET",
          "put" : "100% PUT",
          "read" : "100% seq read",
          "write" : "100% seq write",
          "randread" : "100% random read",
          "randwrite" : "100% random write",
          "get-object-size" : "",
    }

configs = {
            "amcache" : "AM cache 100% hit for data 100% hit for metadata",
            "amcache0" : "AM cache 0% hit for data 100% hit for metadata  - 100% SM and DM cache hit",
            "amcache0-block" : "AM cache 0% hit for data 100% hit for metadata - 100% SM and DM cache hit - New SVC layer off for block",
            "amcache750" : "AM cache 75% hit for data 100% hit for metadata - 100% SM and DM cache hit",
}

def compile_config_notes():
    config_notes = {}
    for t, d1 in test_types.iteritems():
        for m, d2 in mixes.iteritems():
            for c, d3 in configs.iteritems():
                key = t + ":" + m + ":" + c
                config_notes[key] = d1 + " - " + d2 + " - " + d3
    return config_notes

config_notes = compile_config_notes()

def compute_pareto_optimal(iops, lat):
    assert len(iops) == len(lat)
    n = len(iops)
    vector = []
    for i in range(n):
        vector.append(lat[i]/iops[i])
    m = min(vector)
    m_i = vector.index(m)
    return iops[m_i], lat[m_i]

def generate_lat_bw(iops, lat):
    filename = "latbw.png"
    title = "Latency Bandwidth"
    xlabel = "Req/s"
    ylabel = "Latency [ms]"
    plt.figure()
    plt.scatter(iops, lat)
    plt.xlabel(xlabel)
    plt.ylabel(ylabel)
    plt.title(title)
    plt.savefig(filename)
    return filename

def generate_scaling_object_size(objs, bw):
    filename = "scaling_obj_size.png"
    title = "Object size scaling"
    xlabel = "Object size [kB]"
    ylabel = "Bandwidth [MB/s]"
    plt.figure()
    plt.plot(objs, bw)
    plt.xlabel(xlabel)
    plt.ylabel(ylabel)
    plt.title(title)
    plt.savefig(filename)
    return filename


def generate_scaling_iops(conns, iops):
    filename = "scaling_iops.png"
    title = "Req/s"
    xlabel = "Connections"
    ylabel = "Req/s"
    plt.figure()
    plt.plot(conns, iops)
    plt.xlabel(xlabel)
    plt.ylabel(ylabel)
    plt.title(title)
    plt.savefig(filename)
    return filename

def generate_scaling_lat(conns, lat, java_lat, am_lat):
    filename = "scaling_lat.png"
    title = "Latency"
    xlabel = "Connections"
    ylabel = "Latency [ms]"
    plt.figure()
    plt.plot(conns, lat, label="client e2e")
    plt.plot(conns, java_lat, label="xdi e2e")
    plt.plot(conns, am_lat, label="bare\_am e2e")
    #plt.plot(conns, sm_lat, label="sm e2e")
    #plt.plot(conns, dm_lat, label="dm e2e")
    plt.legend()
    plt.xlabel(xlabel)
    plt.ylabel(ylabel)
    plt.title(title)
    plt.savefig(filename)
    return filename

def generate_cpus(conns, cpus):
    filename = "scaling_cpus.png"
    title = "CPU"
    xlabel = "Connections"
    ylabel = "CPU Load [%]"
    plt.figure()
    for k,v in cpus.iteritems():
        plt.plot(conns, v, label=k)
    plt.legend()
    plt.xlabel(xlabel)
    plt.ylabel(ylabel)
    plt.title(title)
    plt.savefig(filename)
    return filename


def mail_success(recipients, images, label):
    preamble = "START"
    epilogue = "END"
    text = "Performance Regressions - %s" % label
    with open(".mail", "w") as _f:
        _f.write(preamble + "\n")
        _f.write("\n")
        _f.write(text + "\n")
        _f.write("\n")
        _f.write(epilogue + "\n")
    # mail_address = "engineering@formationds.com"
    
    attachments = ""
    for e in images:
        attachments += "-a %s " % e
    cmd = "cat .mail | mail -s 'performance regression - %s' %s %s" % (label, attachments, recipients)
    print cmd
    os.system(cmd)

def get_githead(directory):
    return os.popen("git log|head -1|awk '{print $2}'").read().rstrip("\n")

def get_gitbranch(directory):
    return os.popen("git branch --no-color|grep '*'|awk '{print $2}'").read().rstrip("\n")

def get_hostname():
    return os.popen("hostname").read().rstrip("\n")

def mail_summary(summary, images = []):
    githead = get_githead("./")
    gitbranch = get_gitbranch("./")
    hostname = get_hostname()
    preamble = "\
Performance regression\n\
Number of nodes: 1\n\
Branch: %s\n\
hostname: %s\n\
git version: %s\n\
System: Xeon 1S 6-core HT, 32GB DRAM, 1GigE, 12HDD, 2SSDs \n\
---\n\
" % (gitbranch, hostname, githead)
#    epilogue = "\
#Data uploaded to Influxdb @ 10.1.10.222:8083 - login guest - guest - database: perf\n\
#Confluence: https://formationds.atlassian.net/wiki/display/ENG/InfluxDB+for+Performance"
    epilogue = "\n"
    text = "Results:\n"
    table = []
    for t in summary:
#        text += "Config: %s\n" % t
#        text += "Notes: %s\n" % config_notes[t]
#        text += "Req/s: %g\n\
#Latency [ms]: %g\n" % (summary[t]["iops"], summary[t]["lat"])
#        text += "---\n"
        table.append([t, '{0:.2f}'.format(summary[t]["iops"], grouping=True) + ":" '{0:.2f}'.format(summary[t]["iops_stdev"], grouping=True), '{0:.2f}'.format(summary[t]["lat"], grouping=True), summary[t]["test_directory"]])
    headers = ["Config", "Req/s <mean:stdev>", "Latency [ms]", "Test directory"]
    text += tabulate.tabulate(table, headers) + "\n"

    text += "\nConfig Explanation:\n"
    headers = ["Config","Notes"]
    table = []
    for t in summary:
        _tag = reduce(lambda x,y : x+":"+y,t.split(":")[0:3])
        table.append([t, config_notes[_tag]])
    text += tabulate.tabulate(table,headers) + "\n"

    with open(".mail", "w") as _f:
        _f.write(preamble + "\n")
        _f.write("\n")
        _f.write(text + "\n")
        _f.write("\n")
        _f.write(epilogue + "\n")
    # mail_address = "engineering@formationds.com"
    
    attachments = ""
    for e in images:
        attachments += "-a %s " % e
    cmd = "cat .mail | mail -s 'performance regression' %s %s" % (attachments, recipients)
    print cmd
    os.system(cmd)

def mean(vals):
    return sum(vals)/len(vals)

def stdev(vals):
    if len(vals) == 0:
        return 0
    m = mean(vals)
    stdev = 0
    for e in vals:
        stdev += e*e
    stdev /= len(vals)
    stdev -= m*m
    stdev = math.sqrt(stdev) 
    return stdev


if __name__ == "__main__":
    test_db = sys.argv[1]
    tags = sys.argv[2].split(",")
    recipients = sys.argv[3]
    recipients2 = sys.argv[4]
 
    db = dataset.connect('sqlite:///%s' % test_db)
    summary = {}
    for t in tags:
        label = t
        mode, mix, config = t.split(":")[0:3]
        try:
            if mode == "s3" and mix == "get":
                experiments = db["experiments"].find(tag=t)
                experiments = [ x for x in experiments]
                experiments = filter(lambda x : x["type"] == "GET", experiments)
                experiments = sorted(experiments, key = lambda k : int(k["threads"]))
                #iops = [x["th"] for x in experiments]    
                iops_get = [x["am:am_get_obj_req:count"] for x in experiments]    
                #iops_put = [x["am:am_put_obj_req:count"] for x in experiments]
                am_lat = [x["am:am_get_obj_req:latency"] for x in experiments]
                #sm_lat = [x["am:am_get_sm:latency"] for x in experiments]
                sm_lat = []
                #dm_lat = [x["am:am_get_dm:latency"] for x in experiments]
                dm_lat = []
                java_lat = [x["javalat"] for x in experiments]
                cpus = {}
                for a in agents:
                    cpus[a] = [x[a+":cpu"] for x in experiments]
                #iops = [x + y for x,y in zip(*[iops_put, iops_get])]   
                iops = iops_get
                lat = [x["lat"] for x in experiments]    
                #print [x["nreqs"] for x in experiments]    
                conns = [x["threads"] for x in experiments]    

                #max_iops = max(iops)
                if 100 in conns:
                    iops_50 = iops[conns.index(100)]
                    lat_50 = lat[conns.index(100)]
                else:
                    iops_50 = iops[-1]
                    lat_50 = lat[-1]

                test_dir = os.path.dirname([x["test_directory"] for x in experiments][0])
                summary[t] = {"iops" : iops_50, "lat" : lat_50, "test_directory" : test_dir}

                #print [x["type"] for x in experiments]    
                # iops = [x["am:am_get_obj_req:count"] for x in experiments]    
                
                images = [] 
                images.append(generate_scaling_iops(conns, iops))
                images.append(generate_scaling_lat(conns, lat, java_lat, am_lat))
                images.append(generate_cpus(conns, cpus))
                images.append(generate_lat_bw(iops, lat))
                mail_success(recipients2, images, label)
            if mode == "s3" and mix == "get-object-size":
                experiments = db["experiments"].find(tag=t)
                experiments = [ x for x in experiments]
                experiments = filter(lambda x : x["type"] == "GET", experiments)
                experiments = sorted(experiments, key = lambda k : int(k["fsize"]))
                #iops = [x["th"] for x in experiments]    
                iops = [int(x["am:am_get_obj_req:count"]) for x in experiments]    
                bw = [int(x["am:am_get_obj_req:count"])*int(x["fsize"])/1024/1024 for x in experiments]    
                am_lat = [x["am:am_get_obj_req:latency"] for x in experiments]
                #sm_lat = [x["am:am_get_sm:latency"] for x in experiments]
                sm_lat = []
                #dm_lat = [x["am:am_get_dm:latency"] for x in experiments]
                dm_lat = []
                java_lat = [x["javalat"] for x in experiments]
                cpus = {}
                for a in agents:
                    cpus[a] = [x[a+":cpu"] for x in experiments]
                #iops = [x + y for x,y in zip(*[iops_put, iops_get])]   
                lat = [x["lat"] for x in experiments]    
                #print [x["nreqs"] for x in experiments]    
                objs = [int(x["fsize"])/1024 for x in experiments]    
                conns = [x["threads"] for x in experiments]    

                #max_iops = max(iops)
                iops_50 = iops[-1]
                lat_50 = lat[-1]

                test_dir = os.path.dirname([x["test_directory"] for x in experiments][0])
                summary[t] = {"iops" : iops_50, "lat" : lat_50, "test_directory" : test_dir}

                #print [x["type"] for x in experiments]    
                # iops = [x["am:am_get_obj_req:count"] for x in experiments]    
                
                images = [] 
                images.append(generate_scaling_object_size(objs, bw))
                images.append(generate_scaling_lat(conns, lat, java_lat, am_lat))
                images.append(generate_cpus(conns, cpus))
                images.append(generate_lat_bw(iops, lat))
                mail_success(recipients2, images, label)

            if mode == "s3_java" and mix == "get":
                experiments = db["experiments"].find(tag=t)
                experiments = [ x for x in experiments]
                experiments = filter(lambda x : x["type"] == "GET", experiments)
                experiments = sorted(experiments, key = lambda k : int(k["outstanding"]) * int(k["threads"]))
                iops_tester = [x["th"] for x in experiments]    
                iops_get_max = [x["am:am_get_obj_req:count_max"] for x in experiments]    
                iops_get_mean = [x["am:am_get_obj_req:count_mean"] for x in experiments]    
                iops_get_min = [x["am:am_get_obj_req:count_min"] for x in experiments]    
                iops_get_stdev = [x["am:am_get_obj_req:count_stdev"] for x in experiments]    

                am_lat = [x["am:am_get_obj_req:latency"] for x in experiments]
                #sm_lat = [x["am:am_get_sm:latency"] for x in experiments]
                sm_lat = []
                #dm_lat = [x["am:am_get_dm:latency"] for x in experiments]
                dm_lat = []
                java_lat = [x["javalat"] for x in experiments]
                cpus = {}
                for a in agents:
                    cpus[a] = [x[a+":cpu"] for x in experiments]
                lat = [x["lat"] for x in experiments]    
                #print [x["nreqs"] for x in experiments]    
                conns = [int(x["outstanding"]) * int(x["threads"]) for x in experiments]    

                # print indices, [iops_get_mean[i] for i in indices]
                # print indices, [iops_get_max[i] for i in indices]
                # print indices, [iops_get_min[i] for i in indices]
                # print indices, [iops_get_stdev[i] for i in indices]
                # print indices, [iops[i] for i in indices]
                indices = [i for i, x in enumerate(conns) if x == 100]
                iops_100 = max([iops_get_mean[i] for i in indices])
                index = iops_get_mean.index(iops_100)
                assert index in indices
                lat_100 = lat[index]
                # stdev_100 = iops_get_stdev[index] 
                stdev_100 = stdev([iops_get_mean[i] for i in indices])
                test_dir = os.path.dirname([x["test_directory"] for x in experiments][0])
                summary[t] = {
                    "iops" : iops_100, 
                    "lat" : lat_100, 
                    "test_directory" : test_dir, 
                    "iops_max" : mean([iops_get_max[i] for i in indices]), 
                    "iops_min" : mean([iops_get_min[i] for i in indices]), 
                    "iops_stdev" : stdev_100, 
                    "iops_mean" : mean([iops_get_mean[i] for i in indices]),
                    }

                images = [] 
                images.append(generate_scaling_iops(conns, iops_get_mean))
                images.append(generate_scaling_lat(conns, lat, java_lat, am_lat))
                images.append(generate_cpus(conns, cpus))
                images.append(generate_lat_bw(iops_get_mean, lat))
                mail_success(recipients2, images, label)

            if mode == "s3" and mix == "put":
                experiments = db["experiments"].find(tag=t)

                #for e in experiments:
                #    print e
                experiments = [ x for x in experiments]
                experiments = filter(lambda x : x["type"] == "PUT", experiments)
                experiments = sorted(experiments, key = lambda k : int(k["threads"]))
                iops_tester = [x["th"] for x in experiments]    
                #iops_get = [x["am:am_get_obj_req:count"] for x in experiments]    
                iops_put_mean = [x["am:am_put_obj_req:count_mean"] for x in experiments]
                iops_put_max = [x["am:am_put_obj_req:count_max"] for x in experiments]
                iops_put_min = [x["am:am_put_obj_req:count_min"] for x in experiments]
                iops_put_stdev = [x["am:am_put_obj_req:count_stdev"] for x in experiments]
                am_lat = [x["am:am_put_obj_req:latency"] for x in experiments]
                #sm_lat = [x["am:am_get_sm:latency"] for x in experiments]
                sm_lat = []
                #dm_lat = [x["am:am_get_dm:latency"] for x in experiments]
                dm_lat = []
                java_lat = [x["javalat"] for x in experiments]
                cpus = {}
                for a in agents:
                    cpus[a] = [x[a+":cpu"] for x in experiments]
                lat = [x["lat"] for x in experiments]
                conns = [int(x["threads"]) for x in experiments]    
                indices = [i for i, x in enumerate(conns) if x == 50]
                iops_50 = max([iops_put_mean[i] for i in indices])
                index = iops_put_mean.index(iops_50)
                assert index in indices
                lat_50 = lat[index]
                stdev_50 = stdev([iops_put_mean[i] for i in indices])
                test_dir = os.path.dirname([x["test_directory"] for x in experiments][0])
                summary[t] = {
                    "iops" : iops_50, 
                    "lat" : lat_50, 
                    "test_directory" : test_dir, 
                    "iops_max" : mean([iops_put_max[i] for i in indices]), 
                    "iops_min" : mean([iops_put_min[i] for i in indices]), 
                    "iops_stdev" : stdev_50, 
                    "iops_mean" : mean([iops_put_mean[i] for i in indices]),
                    }

                images = [] 
                images.append(generate_scaling_iops(conns, iops_put_mean))
                images.append(generate_scaling_lat(conns, lat, java_lat, am_lat))
                images.append(generate_cpus(conns, cpus))
                images.append(generate_lat_bw(iops_put_mean, lat))
                mail_success(recipients2, images, label)
            elif mode == "fio":
                experiments = db["experiments"].find(tag=t)

                #for e in experiments:
                #    print e
                experiments = [ x for x in experiments]
                experiments = filter(lambda x : x["fio_type"] == "randread", experiments)
                experiments = sorted(experiments, key = lambda k : int(k["iodepth"]))
                iops_tester = [x["th"] for x in experiments]    
                iops_mean = [x["am:am_get_obj_req:count_mean"] for x in experiments]    
                iops_max = [x["am:am_get_obj_req:count_max"] for x in experiments]    
                iops_min = [x["am:am_get_obj_req:count_min"] for x in experiments]    
                iops_stdev = [x["am:am_get_obj_req:count_stdev"] for x in experiments]    
                am_lat = [x["am:am_get_obj_req:latency"] for x in experiments]
                #sm_lat = [x["am:am_get_sm:latency"] for x in experiments]
                sm_lat = []
                #dm_lat = [x["am:am_get_dm:latency"] for x in experiments]
                dm_lat = []
                java_lat = [x["javalat"] for x in experiments]
                cpus = {}
                for a in agents:
                    cpus[a] = [x[a+":cpu"] for x in experiments]

                # iops = [x + y for x,y in zip(*[iops_put, iops_get])]   
                lat = [x["lat"] for x in experiments]    
                #print [x["nreqs"] for x in experiments]    
                conns = [x["iodepth"] for x in experiments]    

                test_dir = os.path.dirname([x["test_directory"] for x in experiments][0])
                # print conns
                # print iops_tester
                # print iops_mean
                # print iops_min
                # print iops_max
                # print iops_stdev
                # print lat
                indices = [i for i, x in enumerate(conns) if x == '128']
                reported_iops = max([iops_tester[i] for i in indices])
                # i_max = iops_tester.index(reported_iops)
                reported_latency = mean([lat[i] for i in indices])
                reported_stdev = stdev([iops_tester[i] for i in indices])
                print reported_stdev

                summary[t] = {
                    "iops" : reported_iops, 
                    "lat" : reported_latency, 
                    "test_directory" : test_dir, 
                    "iops_max" : mean([iops_max[i] for i in indices]), 
                    "iops_min" : mean([iops_min[i] for i in indices]), 
                    "iops_stdev" : reported_stdev, 
                    "iops_mean" : mean([iops_mean[i] for i in indices]),
                    }

                images = [] 
                images.append(generate_scaling_iops(conns, iops_mean))
                images.append(generate_scaling_lat(conns, lat, java_lat, am_lat))
                images.append(generate_cpus(conns, cpus))
                images.append(generate_lat_bw(iops_mean, lat))
                mail_success(recipients2, images, label)
        except Exception, e:
            print "Exception:", e
            traceback.print_exc()
            continue
 
    mail_summary(summary)
    

