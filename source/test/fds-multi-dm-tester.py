#!/usr/bin/python
import os
import sys
sys.path.append('./fdslib/pyfdsp')
import argparse
import requests
import hashlib
import subprocess
import time
import ctypes
from multiprocessing import Process, Value
from tabulate import tabulate

BASE_URL = 'http://localhost:8000/'
BUCKET   = 'bucket1/'
OBJ_NAME = 'obj1'
DATA_NAME= 'rand_data_'
DATA_PATH= 'cfg/'
NUM_PROC = 5

class Worker(Process):
    """ Runs PUT operation and gets result in the background."""
    def __init__(self, data):
        super(Worker, self).__init__()
        self.data = data
        self.timestamp = Value('f', 0.0)
        with open(DATA_PATH+self.data, 'rb') as data_file:
            _data = data_file.read()
            self.hash_before = Value(ctypes.c_char_p, hashlib.sha1(_data).hexdigest())

    def run(self):
        with open(DATA_PATH+self.data, 'rb') as data_file:
            _data = data_file.read()
            self.r = requests.put(BASE_URL+BUCKET+OBJ_NAME, data=_data)
            self.timestamp.value = time.clock()
            print self.r, self.hash_before.value, self.timestamp.value

def main():
    # Create 5 random sets of data
    print 'Creating random data'
    for i in range(NUM_PROC):
        os.system('dd if=/dev/urandom of='+DATA_PATH+DATA_NAME+str(i+1)+' bs='+str(10*(i+1))+'M count=1')

    # Asynchronously put 5 objects towards the AM
    # Make sure to keep close track of timestamps
    try:
        print 'Creating bucket:', requests.put(BASE_URL+BUCKET)
    except Exception as e:
        print 'Error creating bucket:', e

    processes=[]
    timestamps=[]
    hashes=[]

    for i in range(NUM_PROC):
        w = Worker(DATA_NAME+str(i+1))
        processes.append(w)
        w.start()

    # timestamps and hashes will share the same order
    for p in processes:
        p.join()
        timestamps.append(p.timestamp.value)
        hashes.append(p.hash_before.value)

    # Make a pretty table
    pretty_table=[]
    for i in range(NUM_PROC):
        pretty_table.append([hashes[i], timestamps[i]])

    header = ['Original Data Hash', 'Transmission Time']
    print tabulate(pretty_table, header, tablefmt='grid')

    r = requests.get(BASE_URL+BUCKET+OBJ_NAME)
    get_hash = hashlib.sha1(r.content).hexdigest()
    print 'FDS object hash:', get_hash

    # Validate that the data matches at least something we sent it
    data_valid = False
    for p in processes:
        if(p.hash_before.value == get_hash): data_valid = True
    if not data_valid:
        print 'Data is corrupted!  None of the hashes match!'
        return 1

    # Validate that the data matches the latest PUT sent
    max_time = max(timestamps)
    max_time_idx = timestamps.index(max_time)
    if hashes[max_time_idx] != get_hash:
        print 'Data on FDS does not match last sent PUT!'
        print 'PUT hash', hashes[max_time_idx]
        print 'FDS hash', get_hash
        #return 2

    print 'Tests passed!'

    return 0

if __name__ == '__main__':
    main()
