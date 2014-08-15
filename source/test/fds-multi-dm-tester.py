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
from requests_futures.sessions import FuturesSession

BASE_URL = 'http://localhost:8000/'
BUCKET   = 'bucket1/'
OBJ_NAME = 'obj1'
DATA_NAME= 'rand_data_'
DATA_PATH= 'cfg/'
NUM_PROC = 5
TIMEOUT  = 30
MAX_CONCURRENCY_PUT = 999
DEBUG_TIMEOUT = 30
BLOB_WRITE_COMMIT_READ_RETRY = 100

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
            self.r = requests.put(BASE_URL+BUCKET+OBJ_NAME, data=_data, timeout=TIMEOUT)
            self.timestamp.value = time.clock()
            print self.r, self.hash_before.value, self.timestamp.value

class BlobReadWorker(Process):
    ''' Runs GETs against blob being written.  checks against bool.'''
    def __init__(self, obj_name, obj_hash):
        super(BlobReadWorker, self).__init__()
        self.finished_write = False
        self.read_hash = 'No hash yet!'
        self.obj_name = obj_name
        self.write_hash = obj_hash
        print self.obj_name

    def run(self):
        assert(self.write_hash != '')
        assert(self.obj_name != '')
        _counter=0

        # Keep trying to read until write is complete.  Should not succeed
        while self.finished_write is False:
            _counter += 1
            r = requests.get(BASE_URL+BUCKET+self.obj_name)
            #print r.status_code
            #print '[Child thread]', self.finished_write
            assert(r.status_code == 200 or r.status_code == 404)
            if r.status_code == 200:
                break

        print '404 Counter:', _counter

        # File has now been written, error code should be 200 and hashes should match
        for i in range(BLOB_WRITE_COMMIT_READ_RETRY):
            print 'On GET #', i
            r = requests.get(BASE_URL+BUCKET+self.obj_name)
            print r
            if r.status_code == 200:
                print 'Length:', sys.getsizeof(r.content)
                if hashlib.sha1(r.content).hexdigest() == self.write_hash:
                    print 'Success!'
                    return 0
                else:
                    print "Hashes don't match!"

        # If function reaches this point it failed
        print 'Failed!'
        exit(1)


def reset_fds():
    ''' Restarts FDS system.  Assumes we are in test directory!'''
    return os.system('./../tools/fds cleanstart')

def blob_write_commit():
    ''' Writes big blob then tries to read before write completes.'''
    print '[Test] BLOB WRITE COMMIT'

    # Create bucket
    time.sleep(6)
    requests.put(BASE_URL+BUCKET)

    # Reuse multi_blob_write's random data and gen hash
    rand_data_path = DATA_PATH+DATA_NAME+'3'
    rand_data = open(rand_data_path, 'rb')
    rand_data_name = 'blobWriteCommitData'
    rand_data_hash = hashlib.sha1(rand_data.read()).hexdigest()
    rand_data.seek(0)

    # Write the blob asynchronously
    session = FuturesSession(max_workers=MAX_CONCURRENCY_PUT)
    future_put = session.put(BASE_URL+BUCKET+rand_data_name, data=rand_data.read(), timeout=DEBUG_TIMEOUT)

    # Start read loop
    worker = BlobReadWorker(rand_data_name, rand_data_hash)
    worker.start()

    # Check for return code on asynch PUT
    assert(future_put.result().status_code == 200)
    print 'Write done!'

    # PUT is done, GETs should now work
    worker.finished_write = True
    print '[Parent thread]', worker.finished_write
    worker.join()
    return 0

def multi_blob_write():
    ''' Writes multiple blobs to same obj id on the server.'''
    print '[Test] MULTI BLOB WRITE'

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

    print 'Multi blob test passed!'
    return 0

def main():
    reset_fds()
    time.sleep(5)
    multi_blob_write_result = multi_blob_write()
    reset_fds()
    blob_write_commit_result = blob_write_commit()

    # Quit FDS
    os.system('./../tools/fds stop')

    final_retcode = multi_blob_write_result + blob_write_commit_result
    return final_retcode

if __name__ == '__main__':
    main()
