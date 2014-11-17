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

agents = ["am", "xdi", "sm", "dm", "sm", "om"]

config_notes = {
    "s3:get:amcache" : "S3 100% GET (50 connections) - 100% AM cache hit",
    "s3:get:amcache0" : "S3 100% GET (50 connections) - 100% AM cache miss - 100% SM and DM cache hit",
    "s3:put:amcache0" : "S3 100% PUT (30 connections) - 100% AM cache miss - 100% SM and DM cache hit",
    "fio:randread:amcache" : "Block (FIO) 100% Random reads (50 connections) - 100% AM cache size is 1200 entries",
    "fio:randread:amcache0" : "Block (FIO) 100% Random reads (50 connections) - 100% AM cache miss - 100% SM and DM cache hit"
}

def generate_scaling_iops(conns, iops):
    filename = "scaling_iops.png"
    title = "IOPs"
    xlabel = "Connections"
    ylabel = "IOPs"
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
    ylabel = "Latency [ms]"
    plt.figure()
    for k,v in cpus.iteritems():
        plt.plot(conns, v, label=k)
    plt.legend()
    plt.xlabel(xlabel)
    plt.ylabel(ylabel)
    plt.title(title)
    plt.savefig(filename)
    return filename


def mail_success(recipients, images):
    # githead = get_githead(directory)
    # gitbranch = get_gitbranch(directory)
    # hostname = get_hostname()
#    preamble = "\
#Performance regression\n\
#Regression type: %s\n\
#Number of nodes: 1\n\
#Branch: %s\n\
#hostname: %s\n\
#git version: %s\
#" % (regr_type, branch, hostname, githead)
#    epilogue = "\
#Data uploaded to Influxdb @ 10.1.10.222:8083 - login guest - guest - database: perf\n\
#Confluence: https://formationds.atlassian.net/wiki/display/ENG/InfluxDB+for+Performance"
    preamble = "START"
    epilogue = "END"
    text = "Performance Regressions"
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
#        text += "IOPs: %g\n\
#Latency [ms]: %g\n" % (summary[t]["iops"], summary[t]["lat"])
#        text += "---\n"
        table.append([t, str(summary[t]["iops"]), str(summary[t]["lat"])])
    headers = ["Config", "IOPs", "Latency [ms]"]
    text += tabulate.tabulate(table,headers) + "\n"

    text += "\nConfig Explanation:\n"
    headers = ["Config","Notes"]
    table = []
    for t in summary:
        table.append([t, config_notes[t]])
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



if __name__ == "__main__":
    test_db = sys.argv[1]
    tags = sys.argv[2].split(",")
    recipients = sys.argv[3]
    recipients2 = "matteo@formationds.com"
    # directory = sys.argv[1]
    # test_id = int(sys.argv[2])
    # mode = sys.argv[3]
    # branch = sys.argv[4]
 
    db = dataset.connect('sqlite:///%s' % test_db)
    summary = {}
    for t in tags:
        mode, mix, config = t.split(":")
        if mode == "s3" and mix == "get":
            experiments = db["experiments"].find(tag=t)

            #for e in experiments:
            #    print e
            experiments = [ x for x in experiments]
            experiments = filter(lambda x : x["type"] == "GET", experiments)
            experiments = sorted(experiments, key = lambda k : int(k["threads"]))
            #iops = [x["th"] for x in experiments]    
            iops_get = [x["am:am_get_obj_req:count"] for x in experiments]    
            iops_put = [x["am:am_put_obj_req:count"] for x in experiments]
            am_lat = [x["am:am_get_obj_req:latency"] for x in experiments]
            #sm_lat = [x["am:am_get_sm:latency"] for x in experiments]
            sm_lat = []
            #dm_lat = [x["am:am_get_dm:latency"] for x in experiments]
            dm_lat = []
            java_lat = [x["javalat"] for x in experiments]
            cpus = {}
            for a in agents:
                cpus[a] = [x[a+":cpu"] for x in experiments]
            iops = [x + y for x,y in zip(*[iops_put, iops_get])]   
            lat = [x["lat"] for x in experiments]    
            #print [x["nreqs"] for x in experiments]    
            conns = [x["threads"] for x in experiments]    

            #max_iops = max(iops)
            iops_50 = iops[-1]
            lat_50 = lat[-1]
            summary[t] = {"iops" : iops_50, "lat" : lat_50}

            #print [x["type"] for x in experiments]    
            # iops = [x["am:am_get_obj_req:count"] for x in experiments]    
            
            images = [] 
            images.append(generate_scaling_iops(conns, iops))
            images.append(generate_scaling_lat(conns, lat, java_lat, am_lat))
            images.append(generate_cpus(conns, cpus))
            mail_success(recipients2, images)
        if mode == "s3" and mix == "put":
            experiments = db["experiments"].find(tag=t)

            #for e in experiments:
            #    print e
            experiments = [ x for x in experiments]
            experiments = filter(lambda x : x["type"] == "PUT", experiments)
            experiments = sorted(experiments, key = lambda k : int(k["threads"]))
            #iops = [x["th"] for x in experiments]    
            #iops_get = [x["am:am_get_obj_req:count"] for x in experiments]    
            iops_put = [x["am:am_put_obj_req:count"] for x in experiments]
            am_lat = [x["am:am_put_obj_req:latency"] for x in experiments]
            #sm_lat = [x["am:am_get_sm:latency"] for x in experiments]
            sm_lat = []
            #dm_lat = [x["am:am_get_dm:latency"] for x in experiments]
            dm_lat = []
            java_lat = [x["javalat"] for x in experiments]
            cpus = {}
            for a in agents:
                cpus[a] = [x[a+":cpu"] for x in experiments]
            iops = iops_put
            lat = [x["lat"] for x in experiments]
            #print [x["nreqs"] for x in experiments]    
            conns = [x["threads"] for x in experiments]    

            #max_iops = max(iops)
            iops_50 = iops[-1]
            lat_50 = lat[-1]
            summary[t] = {"iops" : iops_50, "lat" : lat_50}

            #print [x["type"] for x in experiments]    
            # iops = [x["am:am_get_obj_req:count"] for x in experiments]    
            
            images = [] 
            images.append(generate_scaling_iops(conns, iops))
            images.append(generate_scaling_lat(conns, lat, java_lat, am_lat))
            images.append(generate_cpus(conns, cpus))
            mail_success(recipients2, images)
        elif mode == "fio":
            experiments = db["experiments"].find(tag=t)

            #for e in experiments:
            #    print e
            experiments = [ x for x in experiments]
            print experiments
            experiments = filter(lambda x : x["fio_type"] == "randread", experiments)
            experiments = sorted(experiments, key = lambda k : int(k["iodepth"]))
            #iops = [x["th"] for x in experiments]    
            iops_get = [x["am:am_get_obj_req:count"] for x in experiments]    
            iops_put = [x["am:am_put_obj_req:count"] for x in experiments]
            am_lat = [x["am:am_get_obj_req:latency"] for x in experiments]
            #sm_lat = [x["am:am_get_sm:latency"] for x in experiments]
            sm_lat = []
            #dm_lat = [x["am:am_get_dm:latency"] for x in experiments]
            dm_lat = []
            java_lat = [x["javalat"] for x in experiments]
            cpus = {}
            for a in agents:
                cpus[a] = [x[a+":cpu"] for x in experiments]

            iops = [x + y for x,y in zip(*[iops_put, iops_get])]   
            lat = [x["lat"] for x in experiments]    
            #print [x["nreqs"] for x in experiments]    
            conns = [x["iodepth"] for x in experiments]    

            #max_iops = max(iops)
            iops_50 = iops[-1]
            lat_50 = lat[-1]
            summary[t] = {"iops" : iops_50, "lat" : lat_50}

            #print [x["type"] for x in experiments]    
            # iops = [x["am:am_get_obj_req:count"] for x in experiments]    
            
            images = [] 
            images.append(generate_scaling_iops(conns, iops))
            images.append(generate_scaling_lat(conns, lat, java_lat, am_lat))
            images.append(generate_cpus(conns, cpus))
            mail_success(recipients2, images)
 
    mail_summary(summary)
    

