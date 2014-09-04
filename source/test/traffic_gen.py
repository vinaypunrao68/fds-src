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

class Histogram:
    def __init__(self, N = 1, M = 0):
        self.bins = [0,] * (N + 1) # FIXME: refactor name
        self.N = N
        self.M = M
        self.intv = float(M)/N
    def add(self, value):
        if value < self.M:
            index = int(value/self.intv)
            self.bins[index] += 1
        else:
            self.bins[self.N] += 1
    def get(self):
        return self.bins
    # def get_bins(self): # TODO
    #    return [ x * self.intv  for x in range(self.N + 1)]
    def update(self, histo):
        vals = histo.get()
        length = len(vals)
        assert length == self.N + 1
        self.bins = [self.bins[i] + vals[i] for i in range(length)]

def init_stats():
    stats = [{"reqs" : 0,
         "fails" : 0,
         "elapsed_time" : 0.0,
         "tot_latency" : 0.0,
         "min_latency" : 1.0e10,   # FIXME: this seems broken
         "max_latency" : 0.0,
         "latency_cnt" : 0,
         "lat_histo"   : Histogram(N = 10, M = 0.010)
    } for v in range(options.num_volumes)]
    #lat_histo = [Histogram() for v in range(options.num_volumes)]
    return stats

def update_stat(stats, name ,val):
    if type(stats[name]) == type(1) or type(stats[name]) == type(1.0):
        stats[name] += val
    else:
        assert type(stats[name]) == type(Histogram())
        stats[name].update(val)

def clear_stats(stats):
    for v in range(options.num_volumes):
        for e in stats:
            e.second = 0

def update_latency_stats(stats, start, end, volume):
    elapsed = end - start
    stats[volume]["latency_cnt"] +=1
    stats[volume]["tot_latency"] += elapsed
    # print "elapsed:",elapsed
    if elapsed < stats[volume]["min_latency"]:
        stats[volume]["min_latency"] = elapsed
    if elapsed > stats[volume]["max_latency"]:
        stats[volume]["max_latency"] = elapsed
    stats[volume]["lat_histo"].add(elapsed)

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
        elif req_type == "7030":
            if random.randint(1,100) < 70:
                # Possibly read a file that been already uploaded
                if len(prev_uploaded[vol]) > 0:
                    file_idx = random.sample(prev_uploaded[vol], 1)[0]
                else:
                    file_idx = random.randint(0, options.num_files - 1)
                e = do_get(conn, "/volume%d/file%d" % (vol, file_idx))
            else:
                # PUT a new file
                file_idx = random.randint(0, options.num_files - 1)
                e = do_put(conn, "/volume%d/file%d" % (vol, file_idx), files[file_idx])
        elif req_type == "NOP":
            time.sleep(.008)
        r1 = conn.getresponse()
        time_end = time.time()
        r1.read()
        # FIXME: need to skip first samples
        update_latency_stats(stats, time_start, time_end, vol)
        stats[vol]["reqs"] += 1
        if r1.status != 200:
            stats[vol]["fails"] += 1
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

def compute_req_per_threads(options):
    reqs_per_thread = [int(options.n_reqs / options.threads) for x in range(options.threads)]
    remainder = options.n_reqs - int(options.n_reqs / options.threads) * options.threads
    i = 0
    while remainder > 0:
        reqs_per_thread[i] += 1
        i += 1
        remainder -= 1
    return reqs_per_thread

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
        # FIXME: compute number of requests per thread here
        reqs_per_thread = compute_req_per_threads(options)
        task_args = (i, reqs_per_thread[i], options.req_type, options.num_volumes, files, stats, queue, part_prev_uploaded[i])
        #t = th.Thread(None,task,"task-"+str(i), task_args)
        t = Process(target=task, args=task_args)
        t.start()
        tids.append(t)
    for t in tids:
        t.join()
    time_end = time.time()
    for v in range(options.num_volumes):
        stats[v]["elapsed_time"] = time_end - time_start
    # FIXME: move to function
    uploaded = [set() for x in range(options.num_volumes)]
    while not queue.empty():
        st, up = queue.get()
        for vol in range(options.num_volumes):
            for k,v in st[vol].items():
                # stats[vol][k] += v
                update_stat(stats[vol], k, v)
            uploaded[vol].update(up[vol])
    # FIXME: volumes (ongoing)
    if options.get_reuse == True and options.req_type == "PUT":
        dump_uploaded(uploaded)
    print "Options:", options, "Stats:", stats
    for vol in range(options.num_volumes):
        print "Summary - volume:", vol, "threads:", options.threads, "n_reqs:", options.n_reqs, "req_type:", options.req_type, \
            "elapsed time:", stats[vol]['elapsed_time'], \
            "reqs/sec:", stats[vol]['reqs'] / stats[vol]['elapsed_time'], \
            "avg_latency[ms]:",stats[vol]["tot_latency"]/stats[vol]["latency_cnt"]*1e3, \
            "failures:", stats[vol]['fails'], "requests:", stats[vol]["reqs"]
        print "Latency histogram:", stats[vol]["lat_histo"].get()

# TODO: reuse on put, sequential mode
# TODO: options to create volume
# TODO: option to reset counters
# TODO: use pools and async: https://docs.python.org/3/library/multiprocessing.html#using-a-pool-of-workers
# TODO: delete
# FIXME: what happen if i use a different numbe rof volumes for put and get?
# TODO: time history of latencies
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
    parser.add_option("-V", "--volume-stats", dest = "volume_stats",  default = False, action = "store_true", help = "Enable per volume stats")

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

