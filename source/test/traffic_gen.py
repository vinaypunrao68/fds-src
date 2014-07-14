#!/usr/bin/python
import os,sys,re
import httplib
import time
import threading as th
from optparse import OptionParser
import tempfile
from Queue import *

stats = {"reqs" : 0,
	 "fails" : 0,
	 "elapsed_time" : 0.0,
	 "tot_latency" : 0.0,
	 "min_latency" : 1.0e10,
	 "max_latency" : 0.0,
         "latency_cnt" : 0
}

def clear_stats():
	for e in stats:
		e.second = 0

def update_latency_stats(start, end):
	elapsed = end - start
	stats["latency_cnt"] +=1
	stats["tot_latency"] += elapsed
	if elapsed < stats["min_latency"]: 
		stats["min_latency"] = elapsed
	if elapsed > stats["max_latency"]: 
		stats["max_latency"] = elapsed

def create_random_file(size):
	fout, fname = tempfile.mkstemp()
	#fout  = open('output_file', 'wb') 
    	os.write(fout, os.urandom(size))
	os.close(fout)
	return fname 

def create_file_queue(n,size):
	q = Queue()
	for i in range(0,n):
		q.put(create_random_file(size))
	return q

def do_put(conn, target, fname):
	body = open(fname,"r").read()
	conn.request("PUT", target, body)

def do_get(conn, target):
	conn.request("GET", target)

def do_delete(conn, target):
	conn.request("DELETE", target)

def task(task_id, n_reqs, req_type, file_q):
	conn = httplib.HTTPConnection("localhost:8000")
	for i in range(0,n_reqs):
		#conn.request("PUT", "/volume")
		time_start = time.time()
		if req_type == "PUT":
			do_put(conn, "/volume/file%d:%d" % (task_id,i), files.get())
			files.task_done()
		elif req_type == "GET":
			do_get(conn, "/volume/file%d:%d" % (task_id,i))
		elif req_type == "DELETE":
			do_delete(conn, "/volume/file%d:%d" % (task_id,i))
		r1 = conn.getresponse()
		time_end = time.time()
		update_latency_stats(time_start, time_end)
		data = r1.read()
		stats["reqs"] += 1
		if r1.status != 200:
			stats["fails"] += 1
		#print r1.status, r1.reason, data
		#print r1.status, r1.reason, data
		#conn.request("GET", "/volume/file")
		#r1 = conn.getresponse()
		#data = r1.read()
		#print r1.status, r1.reason, data
	#body.close()
	conn.close()

def main(options,files):
	#body = create_random_file(options.file_size)
	#body = open(body.name,"r")
	#text = body.read()
	tids = []
	for i in range(0,options.threads):
		task_args = [i, options.n_reqs, options.req_type, files]
		t = th.Thread(None,task,"task-"+str(i), task_args)
		t.start()
		tids.append(t)
	for t in tids:
		t.join()

if __name__ == "__main__":
	parser = OptionParser()
	parser.add_option("-t", "--threads", dest = "threads", type = "int", default = 1, help = "Number of threads")
	parser.add_option("-n", "--num-requests", dest = "n_reqs", type = "int", default = 1, help = "Number of requests per thread")
	parser.add_option("-T", "--type", dest = "req_type", default = "PUT", help = "PUT/GET/DELETE")
	parser.add_option("-s", "--file_size", dest = "file_size", type = "int", default = 1024, help = "File size in bytes")
	(options, args) = parser.parse_args()
	if options.req_type == "PUT":
		print "Creating files"
		files = create_file_queue(options.threads * options.n_reqs, options.file_size)
	else:
		files = Queue()
	print "Starting"
	time_start = time.time()
	main(options,files)
	time_end = time.time()
	stats["elapsed_time"] = time_end - time_start
	print "threads:", options.threads, "n_reqs:", options.n_reqs, "req_type:", options.req_type, "elapsed time:", (time_end - time_start), "reqs/sec:", options.threads * options.n_reqs / (time_end - time_start), "avg_latency[ms]:",stats["tot_latency"]/stats["latency_cnt"]*1e3
	print "Options:", options,"Stats:", stats
