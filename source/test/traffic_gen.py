#!/usr/bin/python
import os,sys,re
import time
#import threading as th
from multiprocessing import Process, Queue
from optparse import OptionParser
import tempfile
#from Queue import *
import httplib
import requests
import random

#HTTP_LIB = "requests"
HTTP_LIB = "httplib"

def init_stats():
    stats = {"reqs" : 0,
         "fails" : 0,
         "elapsed_time" : 0.0,
         "tot_latency" : 0.0,
         "min_latency" : 1.0e10,
         "max_latency" : 0.0,
         "latency_cnt" : 0
    }
    return stats

def clear_stats():
    for e in stats:
        e.second = 0

def update_latency_stats(stats, start, end):
    elapsed = end - start
    stats["latency_cnt"] +=1
    stats["tot_latency"] += elapsed
    if elapsed < stats["min_latency"]: 
        stats["min_latency"] = elapsed
    if elapsed > stats["max_latency"]: 
        stats["max_latency"] = elapsed

def create_random_file(size):
    fout, fname = tempfile.mkstemp(prefix="fdstrgen")
    #fout  = open('output_file', 'wb') 
    os.write(fout, os.urandom(size))
    os.close(fout)
    return fname 

def create_file_queue(n,size):
    files = []
    for i in range(0,n):
        files.append(create_random_file(size))
    return files

def do_put(conn, target, fname):
    e = None
    body = open(fname,"r").read()
    if HTTP_LIB == "httplib":
        conn.request("PUT", target, body)
    elif HTTP_LIB == "requests":
        e = requests.put("http://localhost:8000" + target, data = body)
    return e    

def do_get(conn, target):
    e = None
    if HTTP_LIB == "httplib":
        conn.request("GET", target)
    elif HTTP_LIB == "requests":
        e = requests.get("http://localhost:8000" + target)
    return e    

def do_delete(conn, target):
    e = None
    if HTTP_LIB == "httplib":
        conn.request("DELETE", target)
    elif HTTP_LIB == "requests":
        e = requests.delete("http://localhost:8000" + target)
    return e    

def task(task_id, n_reqs, req_type, files, stats, queue):
    if HTTP_LIB == "httplib":
        conn = httplib.HTTPConnection("localhost:8000")
    else:
        conn = None
    for i in range(0,n_reqs):
        #conn.request("PUT", "/volume")
        time_start = time.time()
        file_idx = random.randint(0, options.num_files - 1)
        if req_type == "PUT":
            e = do_put(conn, "/volume/file%d" % (file_idx), files[file_idx])
            #files.task_done()
        elif req_type == "GET":
            e = do_get(conn, "/volume/file%d" % (file_idx))
        elif req_type == "DELETE":
            e = do_delete(conn, "/volume/file%d" % (file_idx))
        elif req_type == "7030":
            if random.randint(1,100) < 70:
                e = do_get(conn, "/volume/file%d" % (file_idx))
            else: 
                e = do_put(conn, "/volume/file%d" % (file_idx), files[file_idx])
        if HTTP_LIB == "httplib":
            r1 = conn.getresponse()
            r1.read()
        time_end = time.time()
        update_latency_stats(stats, time_start, time_end)
        stats["reqs"] += 1
        if HTTP_LIB == "httplib":
            if r1.status != 200:
                stats["fails"] += 1
        elif HTTP_LIB == "requests":
            if e.status_code != 200:        
                stats["fails"] += 1
        #print r1.status, r1.reason, data
        #print r1.status, r1.reason, data
        #conn.request("GET", "/volume/file")
        #r1 = conn.getresponse()
        #data = r1.read()
        #print r1.status, r1.reason, data
    #body.close()
    if HTTP_LIB == "httplib":
        conn.close()
    queue.put(stats)


def main(options,files):
    #body = create_random_file(options.file_size)
    #body = open(body.name,"r")
    #text = body.read()
    queue = Queue()
    stats = init_stats()
    tids = []
    for i in range(0,options.threads):
        task_args = (i, options.n_reqs, options.req_type, files, stats, queue)
        #t = th.Thread(None,task,"task-"+str(i), task_args)
        t = Process(target=task, args=task_args)
        t.start()
        tids.append(t)
    for t in tids:
        t.join()
    while not queue.empty():
        e = queue.get()
        for k,v in e.items():
            stats[k] += v
    return stats

if __name__ == "__main__":
    parser = OptionParser()
    parser.add_option("-t", "--threads", dest = "threads", type = "int", default = 1, help = "Number of threads")
    parser.add_option("-n", "--num-requests", dest = "n_reqs", type = "int", default = 1, help = "Number of requests per thread")
    parser.add_option("-T", "--type", dest = "req_type", default = "PUT", help = "PUT/GET/DELETE")
    parser.add_option("-s", "--file-size", dest = "file_size", type = "int", default = 4096, help = "File size in bytes")
    parser.add_option("-F", "--num-files", dest = "num_files", type = "int", default = 10000, help = "File size in bytes")
    (options, args) = parser.parse_args()
    if options.req_type == "PUT" or options.req_type == "7030":
        print "Creating files"
        files = create_file_queue(options.num_files, options.file_size)
        time.sleep(5)
    else:
        files = Queue()
    print "Starting"
    time_start = time.time()
    stats = main(options,files)
    time_end = time.time()
    stats["elapsed_time"] = time_end - time_start
    print "Summary - threads:", options.threads, "n_reqs:", options.n_reqs, "req_type:", options.req_type, "elapsed time:", (time_end - time_start), "reqs/sec:", stats['reqs'] / stats['elapsed_time'], "avg_latency[ms]:",stats["tot_latency"]/stats["latency_cnt"]*1e3, "failures:", stats['fails'], "requests:", stats["reqs"]
    print "Options:", options,"Stats:", stats
