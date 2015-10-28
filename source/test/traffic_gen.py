#!/usr/bin/python3
import os, sys, re
import time
import threading
from multiprocessing import Process, Queue, JoinableQueue, Barrier, Array
from optparse import OptionParser
import tempfile
# from Queue import *
import http.client
# import requests
import random
import pickle
# For multipart uploads
import math
import boto
from boto.s3.connection import OrdinaryCallingFormat
from boto.s3.key import Key
from boto.s3.bucket import Bucket
from filechunkio import FileChunkIO


class Histogram:
    def __init__(self, N=1, M=0):
        self.bins = [0, ] * (N + 1)  # FIXME: refactor name
        self.N = N
        self.M = M
        self.intv = float(M) / N

    def add(self, value):
        if value < self.M:
            index = int(value / self.intv)
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
    stats = {"reqs": 0,
             "fails": 0,
             "elapsed_time": 0.0,
             "tot_latency": 0.0,
             "min_latency": 1.0e10,  # FIXME: this seems broken
             "max_latency": 0.0,
             "latency_cnt": 0,
             "lat_histo": Histogram(N=10, M=0.010),
             # "lat_histo"   : Histogram(N = 100, M = 10.0),
             "inst_reqs": 0}
    # lat_histo = [Histogram() for v in range(options.num_volumes)]
    return stats


def update_stat(stats, name, val):
    if type(stats[name]) == type(1) or type(stats[name]) == type(1.0):
        stats[name] += val
    else:
        assert type(stats[name]) == type(Histogram())
        stats[name].update(val)


def reset_cntr(stats, name):
    stats[name] = 0


def reset_reqs(stats, nvols, q):
    for i in range(nvols):
        reset_cntr(stats, "inst_reqs")


def start_reset_reqs(stats, nvols):
    q = Queue()
    t = threading.Thread(target=reset_reqs, args=(stats, nvols, q))
    t.start()
    return t


def update_latency_stats(stats, start, end):
    elapsed = end - start
    stats["latency_cnt"] += 1
    stats["tot_latency"] += elapsed
    # print "elapsed:",elapsed
    if elapsed < stats["min_latency"]:
        stats["min_latency"] = elapsed
    if elapsed > stats["max_latency"]:
        stats["max_latency"] = elapsed
    stats["lat_histo"].add(elapsed)


def create_random_file(size):
    fout, fname = tempfile.mkstemp(prefix="fdstrgen")
    # fout  = open('output_file', 'wb')
    os.write(fout, os.urandom(size))
    os.close(fout)
    return fname


def create_file_queue(n, size):
    files = []
    for i in range(0, n):
        files.append(create_random_file(size))
    return files


def validate_files(file1, file2):
    diff_code = os.system("diff " + file1 + " " + file2)
    #print("%s || %s --> %d" % (file1, file2, diff_code))
    return diff_code

def do_multipart(conn, target_volume, target_file, fname, fsize):
    # setup for s3 connection sample from fds-s3-compliance.py
    # print("%s --> %s/%s" % (fname, target_volume, target_file))
    buck = conn.create_bucket(target_volume)

    mp = buck.initiate_multipart_upload(target_file)

    # chunk size 10MB
    # TODO Make chunk_size variable
    chunk_size = 1024 * 1024 * 10
    chunk_count = int(math.ceil(fsize / chunk_size))

    # Upload file in parts
    for i in range(chunk_count + 1):
        offset = chunk_size * i
        bytes = min(chunk_size, fsize - offset)
        with FileChunkIO(fname, 'r', offset=offset, bytes=bytes) as fp:
            mp.upload_part_from_file(fp, part_num=i + 1)

    mp.complete_upload()

    # Validate Upload
    _key = Key(buck)
    _key.key = target_file
    _key.get_contents_to_filename(target_file)
    if validate_files(fname, target_file) != 0:
        raise Exception('File corrupted on Multipart Upload!')

    # Clean old files (save space)
    os.system("rm " + target_file)
    # os.system("rm " + fname)


def do_put(conn, target_vol, target_file, fname):
    buck = conn.create_bucket(target_vol)
    _key = Key(buck)
    _key.key = target_file
    ret = _key.set_contents_from_filename(fname)
    if ret != 4096:
        print(target_file)

    # Validate Upload
    _key.get_contents_to_filename(target_file)
    if validate_files(fname, target_file) != 0:
        raise Exception('File corrupted on Put!')
    conn.close()

    os.system("rm " + target_file)

def do_get(conn, target_vol, target_file):
    buck = conn.get_bucket(target_vol)
    nonexistant = conn.lookup(target_vol)
    if nonexistant is None:
        print("No such bucket!!")
    # print(target_file)
    # for k in buck.list():
    #    print(k.name)
    _key = Key(buck)
    _key.key = target_file
    fout, fname = tempfile.mkstemp(prefix="fdstrgen")
    _key.get_contents_to_filename(fname)
    os.close(fout)


def do_delete(conn, target_vol, target_file):
    buck = conn.get_bucket(target_vol)
    _key = Key(buck)
    _key.key = target_file
    _key.delete()


def task(task_id, n_reqs, req_type, vol, files,
         queue, prev_uploaded, barrier, counters, time_start_volume):
    used_files = set()
    stats = dict(init_stats())
    n = barrier.wait()
    # print("id:%d || barrier:%d" % (task_id,n))
    error = 0
    if task_id == 0:
        time_start_volume[vol] = time.time()
        print ("starting timer -  thread:", task_id, "volume", vol, "time:", time_start_volume[vol])
    uploaded = set()
    conn = boto.connect_s3(
        aws_access_key_id='admin',
        aws_secret_access_key='admin',
        host='localhost',
        port=8000,
        is_secure=False,
        calling_format=boto.s3.connection.OrdinaryCallingFormat())
    # print("TaskID:", task_id, " || Volume:", vol)
    for i in range(0, n_reqs):
        if options.heartbeat > 0 and i % options.heartbeat == 0:
            print ("heartbeat for", task_id, "volume_id:", vol, "i:", i)
        time_start = time.time()
        if req_type == "PUT":
            if options.put_seq is True:
                file_idx = (stats["reqs"] + int(options.n_reqs / options.threads) * task_id) % options.num_files
                assert not (file_idx in used_files)
                used_files.add(file_idx)
            elif options.put_duplicate is True:
                file_idx = i % options.num_files
            else:
                file_idx = random.randint(0, options.num_files - 1)
            uploaded.add(file_idx)
            # print "PUT", file_idx
            try:
                # print("Put File on /volume%d/file%d-%d" % (vol, file_idx, task_id))
                do_put(conn, "volume%d" % vol, "file%d-%d" % (file_idx, task_id), files[file_idx])
            except:
                print("Put File corrupted on /volume%d/file%d-%d" % (vol, file_idx, task_id))
                error = 1
            # files.task_done()
        elif req_type == "MULTIPART":
            if options.put_seq is True:
                file_idx = (stats["reqs"] + int(options.n_reqs / options.threads) * task_id) % options.num_files
                assert not (file_idx in used_files)
                used_files.add(file_idx)
            elif options.put_duplicate is True:
                file_idx = i % options.num_files
            else:
                file_idx = random.randint(0, options.num_files - 1)
            uploaded.add(file_idx)
            try:
                do_multipart(conn, "volume%d" % vol, "file%d-%d-big" % (file_idx, task_id), files[file_idx],
                                 options.file_size)
            except:
                print("Multipart File Corrupted on /volume%d/file%d-%d-big" % (vol, file_idx, task_id))
                error = 1
        elif req_type == "GET":
            if len(prev_uploaded[vol]) > 0:
                file_idx = random.sample(prev_uploaded[vol], 1)[0]
            else:
                file_idx = random.randint(0, options.num_files - 1)
            # print "GET", file_idx
            do_get(conn, "volume%d" % vol, "file%d-%d" % (file_idx, task_id))
        elif req_type == "DELETE":
            if len(prev_uploaded[vol]) > 0:
                file_idx = random.sample(prev_uploaded[vol], 1)[
                    0]  # FIXME: this works only once, then uploaded should be cleared/updated
            else:
                file_idx = random.randint(0, options.num_files - 1)
            do_delete(conn, "volume%d" % (vol), "file%d-%d-%d" % (file_idx, task_id, n))
        elif req_type == "7030":
            if random.randint(1, 100) < 70:
                # Possibly read a file that been already uploaded
                if len(prev_uploaded[vol]) > 0:
                    file_idx = random.sample(prev_uploaded[vol], 1)[0]
                else:
                    file_idx = random.randint(0, options.num_files - 1)
                do_get(conn, "volume%d" % vol, "file%d" % file_idx)
            else:
                # PUT a new file
                file_idx = random.randint(0, options.num_files - 1)
                do_put(conn, "volume%d" % vol, "file%d" % file_idx, files[file_idx])
        elif req_type == "NOP":
            time.sleep(.008)
        time_end = time.time()
        update_latency_stats(stats, time_start, time_end)
        if error == 1:
            stats["fails"] += 1
            error = 0
        else:
            stats["reqs"] += 1

    conn.close()
    # print ("Done with volume", vol, "thread:", task_id)
    with counters.get_lock():
        counters[vol] += 1
        if counters[vol] == options.threads:
            print ("Done with volume:", vol, "thread:", task_id, "end_time:", time.time(), "counters:", counters[vol])
            counters[vol] = -1
            stats["elapsed_time"] = time.time() - time_start_volume[vol]
    time.sleep(1)
    queue.put((stats, uploaded, vol, used_files))


# TODO: add volumes here and ...
def load_previous_uploaded():
    part_prev_uploaded = [[set() for x in range(options.num_volumes)] for y in range(options.threads)]
    # TODO: make a function
    if os.path.exists(".uploaded.pickle"):
        prev_uploaded_file = open(".uploaded.pickle", "rb")
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
    uploaded_file = open(".uploaded.pickle", "wb")
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


class AtomicCounter:
    def __init__(self):
        self.lock = Lock()
        self.counter = Value("i", 0)

    def get(self):
        self.lock.acquire()
        val = self.counter.value
        self.lock.release()
        return val

    def inc(self):
        self.lock.acquire()
        self.counter += 1
        self.lock.release()


def main(options, files):
    # unpickle uploaded files
    if options.get_reuse == True and options.req_type != "PUT":
        part_prev_uploaded = load_previous_uploaded()
    else:
        part_prev_uploaded = [[set() for x in range(options.num_volumes)] for y in range(options.threads)]
    queue = JoinableQueue()
    barrier = Barrier(options.threads * options.num_volumes)
    # tids = [[] for x in range(options.num_volumes)]
    tids = []
    # time_start = time.time()
    reqs_per_thread = compute_req_per_threads(options)
    # counters = [Value("i", 0) for x in range(options.num_volumes)]
    counters = Array("i", [0, ] * options.num_volumes)
    time_start_volume = Array("d", [0.0, ] * options.num_volumes)
    for i in range(0, options.threads):
        for v in range(options.num_volumes):
            # make sure each task has a unique thread id
            task_args = (i*options.threads+v, reqs_per_thread[i], options.req_type, v, files,
                         queue, part_prev_uploaded[i], barrier, counters,
                         time_start_volume)
            t = Process(target=task, args=task_args)
            t.start()
            tids.append(t)
    # Wait until all processes are done with work
    wait = True
    while wait:
        with counters.get_lock():
            for v in range(options.num_volumes):
                # print ("counters: ", v, counters[v],)
                wait = wait & (counters[v] > -1)
            # print (" ->", wait)
            time.sleep(.1)
    uploaded = [set() for x in range(options.num_volumes)]
    stats = [init_stats() for x in range(options.num_volumes)]
    # wait until all processes have pushed their stats into the queue
    # print ("qsize:", queue.qsize())
    while queue.qsize() < (options.num_volumes * options.threads):
        # print (".qsize:", queue.qsize())
        time.sleep(1)  # FIXME: this delay need to be large enough, likely some race here
    time.sleep(1)
    # pull and aggregate stats
    used_files = set()
    while not queue.empty():
        st, up, vol, used = queue.get()
        if options.put_seq is True:
            assert len(used_files.intersection(used)) == 0
            used_files.update(used)
        # print ("updating")
        for k, v in st.items():
            # print (".")
            update_stat(stats[vol], k, v)
        uploaded[vol].update(up)
        queue.task_done()
    # print ("queue is empty, cnt:", cnt)
    # Finally, join processes
    for t in tids:
        t.join()

    if options.get_reuse == True and options.req_type == "PUT":
        dump_uploaded(uploaded)
    print ("Options:", options, "Stats:", stats)
    for vol in range(options.num_volumes):
        print (
        "Summary - volume:", vol, "threads:", options.threads, "n_reqs:", options.n_reqs, "req_type:", options.req_type, \
        "elapsed time:", stats[vol]['elapsed_time'], \
        "reqs/sec:", stats[vol]['reqs'] / stats[vol]['elapsed_time'], \
        "avg_latency[ms]:", stats[vol]["tot_latency"] / stats[vol]["latency_cnt"] * 1e3, \
        "failures:", stats[vol]['fails'], "requests:", stats[vol]["reqs"])
        print ("Latency histogram:", stats[vol]["lat_histo"].get())


# TODO: reuse on put, sequential mode
# TODO: options to create volume
# TODO: option to reset counters
# TODO: use pools and async: https://docs.python.org/3/library/multiprocessing.html#using-a-pool-of-workers
# TODO: delete
# FIXME: what happen if i use a different number of volumes for put and get?
# TODO: time history of latencies
# TODO: Add 
if __name__ == "__main__":
    parser = OptionParser()
    parser.add_option("-t", "--threads", dest="threads", type="int", default=1, help="Number of threads")
    # FIXME: specify total number of request
    parser.add_option("-n", "--num-requests", dest="n_reqs", type="int", default=1,
                      help="Number of requests per volume")
    parser.add_option("-T", "--type", dest="req_type", default="PUT", help="PUT/GET/DELETE/MULTIPART")
    parser.add_option("-s", "--file-size", dest="file_size", type="int", default=4096, help="File size in bytes")
    parser.add_option("-F", "--num-files", dest="num_files", type="int", default=10000, help="Number of files")
    parser.add_option("-v", "--num-volumes", dest="num_volumes", type="int", default=1, help="Number of volumes")
    parser.add_option("-u", "--get-reuse", dest="get_reuse", default=False, action="store_true",
                      help="Do gets on file previously uploaded with put")
    parser.add_option("-b", "--heartbeat", dest="heartbeat", type="int", default=0,
                      help="Heartbeat. Default is no heartbeat (0)")
    parser.add_option("-N", "--target-node", dest="target_node", default="localhost",
                      help="Target node (default is localhost)")
    parser.add_option("-P", "--target-port", dest="target_port", type="int", default=8000,
                      help="Target port (default is 8000)")
    parser.add_option("-V", "--volume-stats", dest="volume_stats", default=False, action="store_true",
                      help="Enable per volume stats")
    parser.add_option("-S", "--put-seq", dest="put_seq", default=False, action="store_true",
                      help="Generate put objects sequentially and uniquely")
    parser.add_option("-D", "--put-duplicate", dest="put_duplicate", default=False, action="store_true",
                      help="Generate duplicate put objects in each volume")

    (options, args) = parser.parse_args()
    # If multipart and filesize is less than 5MB - return error
    if options.req_type == "MULTIPART" and options.file_size < (1024 * 1024 * 10):
        raise Exception("Filesize for Multipart upload must be greater than 10MB")
    if options.req_type == "PUT" or options.req_type == "7030":
        print ("Creating files")
        files = create_file_queue(options.num_files, options.file_size)
        time.sleep(5)
    elif options.req_type == "MULTIPART":
        print ("Creating files")
        files = create_file_queue(options.num_files, options.file_size)
        time.sleep(5)
    else:
        files = Queue()
    print ("Starting...")
    main(options, files)
    print ("Done.")
