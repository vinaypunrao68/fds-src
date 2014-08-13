#!/usr/bin/python
import os,sys,re
import time
# import threading as th
from multiprocessing import Process, Queue
from optparse import OptionParser
import tempfile
# from Queue import *
import httplib
# import requests
import random
import pickle

def init_stats():
    stats = {"reqs" : 0,
         "fails" : 0,
         "elapsed_time" : 0.0,
         "tot_latency" : 0.0,
         "min_latency" : 1.0e10,   # FIXME: this seems broken
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
    conn.request("PUT", target, body)
    return e

def do_get(conn, target):
    e = None
    conn.request("GET", target)
    return e

def do_delete(conn, target):
    e = None
    conn.request("DELETE", target)
    return e

def task(task_id, n_reqs, req_type, nvols, files, stats, queue, prev_uploaded):
    uploaded = [set() for x in range(options.num_volumes)]
    conn = httplib.HTTPConnection(options.target_node + ":" + str(options.target_port))
    for i in range(0,n_reqs):
        if options.heartbeat > 0 and  i % options.heartbeat == 0:
            print "heartbeat for", task_id, "i:", i
        vol = random.randint(0, nvols - 1)
        time_start = time.time()
        if req_type == "PUT":
            file_idx = random.randint(0, options.num_files - 1)
            uploaded[vol].add(file_idx)
            # print "PUT", file_idx
            e = do_put(conn, "/volume%d/file%d" % (vol, file_idx), files[file_idx])
            #files.task_done()
        elif req_type == "GET":
            if len(prev_uploaded[vol]) > 0:
                file_idx = random.sample(prev_uploaded[vol], 1)[0]
            else:
                file_idx = random.randint(0, options.num_files - 1)
            # print "GET", file_idx
            e = do_get(conn, "/volume%d/file%d" % (vol, file_idx))
        elif req_type == "DELETE":
            if len(prev_uploaded[vol]) > 0:
                file_idx = random.sample(prev_uploaded[vol], 1)[0]  # FIXME: this works only once, then uploaded should be cleared/updated
            else:
                file_idx = random.randint(0, options.num_files - 1)
            e = do_delete(conn, "/volume%d/file%d" % (vol, file_idx))
        elif req_type == "7030":  # TODO: this does not work with get_reuse
            if random.randint(1,100) < 70:
                file_idx = random.randint(0, options.num_files - 1)
                e = do_get(conn, "/volume%d/file%d" % (vol, file_idx))
            else:
                file_idx = random.randint(0, options.num_files - 1)
                e = do_put(conn, "/volume%d/file%d" % (vol, file_idx), files[file_idx])
        elif req_type == "NOP":
            time.sleep(.008)
        r1 = conn.getresponse()
        r1.read()
        time_end = time.time()
        update_latency_stats(stats, time_start, time_end)
        stats["reqs"] += 1
        if r1.status != 200:
            stats["fails"] += 1
    conn.close()
    queue.put((stats, uploaded))

# TODO: add volumes here and ...
def load_previous_uploaded():
    part_prev_uploaded = [[set() for x in range(options.num_volumes)] for y in range(options.threads)]
    # TODO: make a function
    if os.path.exists(".uploaded.pickle"):
        prev_uploaded_file = open(".uploaded.pickle", "r")
        upk = pickle.Unpickler(prev_uploaded_file)
        prev_uploaded = upk.load()
        for j in range(options.num_volumes):
            assert len(prev_uploaded[j]) > options.threads
            i = 0
            while len(prev_uploaded[j]) > 0:
                e = prev_uploaded[j].pop()
                part_prev_uploaded[i][j].add(e)
                i = (i + 1) % options.threads
    return part_prev_uploaded

def dump_uploaded(uploaded):
        # pickle uploaded files
        uploaded_file = open(".uploaded.pickle", "w")
        pk = pickle.Pickler(uploaded_file)
        pk.dump(uploaded)
        uploaded_file.close()

def main(options,files):
    # unpickle uploaded files
    if options.get_reuse == True and options.req_type != "PUT":
        part_prev_uploaded = load_previous_uploaded()
    else:
        part_prev_uploaded = [[set() for x in range(options.num_volumes)] for y in range(options.threads)]

    queue = Queue()
    stats = init_stats()
    tids = []
    time_start = time.time()
    for i in range(0,options.threads):
        task_args = (i, options.n_reqs, options.req_type, options.num_volumes, files, stats, queue, part_prev_uploaded[i])
        #t = th.Thread(None,task,"task-"+str(i), task_args)
        t = Process(target=task, args=task_args)
        t.start()
        tids.append(t)
    for t in tids:
        t.join()
    time_end = time.time()
    stats["elapsed_time"] = time_end - time_start
    # FIXME: move to function
    uploaded = [set() for x in range(options.num_volumes)]
    while not queue.empty():
        st, up = queue.get()
        for k,v in st.items():
            stats[k] += v
        for i in range(options.num_volumes):
            uploaded[i].update(up[i])
    # FIXME: volumes (ongoing)
    if options.get_reuse == True and options.req_type == "PUT":
        dump_uploaded(uploaded)
    print "Options:", options,"Stats:", stats
    print "Summary - threads:", options.threads, "n_reqs:", options.n_reqs, "req_type:", options.req_type, "elapsed time:", (time_end - time_start), "reqs/sec:", stats['reqs'] / stats['elapsed_time'], "avg_latency[ms]:",stats["tot_latency"]/stats["latency_cnt"]*1e3, "failures:", stats['fails'], "requests:", stats["reqs"]

# TODO: reuse on put, sequential mode
# TODO: options to create volume
# TODO: option to reset counters
# Delete test
# 7030 test

if __name__ == "__main__":
    parser = OptionParser()
    parser.add_option("-t", "--threads", dest = "threads", type = "int", default = 1, help = "Number of threads")
    # FIXME: specify total number of request
    parser.add_option("-n", "--num-requests", dest = "n_reqs", type = "int", default = 1, help = "Number of requests per thread")
    parser.add_option("-T", "--type", dest = "req_type", default = "PUT", help = "PUT/GET/DELETE")
    parser.add_option("-s", "--file-size", dest = "file_size", type = "int", default = 4096, help = "File size in bytes")
    parser.add_option("-F", "--num-files", dest = "num_files", type = "int", default = 10000, help = "Number of files")
    parser.add_option("-v", "--num-volumes", dest = "num_volumes", type = "int", default = 1, help = "Number of volumes")
    parser.add_option("-u", "--get-reuse", dest = "get_reuse", default = False, action = "store_true", help = "Do gets on file previously uploaded with put")
    parser.add_option("-b", "--heartbeat", dest = "heartbeat", type = "int", default = 0, help = "Heartbeat. Default is no heartbeat (0)")
    parser.add_option("-N", "--target-node", dest = "target_node", default = "localhost", help = "Target node (default is localhost)")
    parser.add_option("-P", "--target-port", dest = "target_port", type = "int", default = 8000, help = "Target port (default is 8000)")

    (options, args) = parser.parse_args()
    if options.req_type == "PUT" or options.req_type == "7030":
        print "Creating files"
        files = create_file_queue(options.num_files, options.file_size)
        time.sleep(5)
    else:
        files = Queue()
    print "Starting..."
    main(options,files)
    print "Done."

