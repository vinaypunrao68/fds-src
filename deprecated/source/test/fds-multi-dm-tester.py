#!/usr/bin/python
import os
import sys
sys.path.append('./fdslib/pyfdsp')
import argparse
import requests
import hashlib
import threading
import subprocess
import time
import fileinput
import ctypes
from multiprocessing import Process, Value
from tabulate import tabulate
from requests_futures.sessions import FuturesSession

DEBUG = False
REM_CFG = ''
DEFAULT_COMMIT_LOG = 'commit_log_size = 5242880'
NUM_PUTS = 10
SLEEP = 5
BASE_URL = None
BUCKET   = 'bucket1/'
OBJ_NAME = 'obj1'
DATA_NAME= 'rand_data_'
REMOTE = False
DATA_PATH= 'cfg/'
NUM_PROC = 5
TIMEOUT  = 60
MAX_CONCURRENCY_PUT = 999
DEBUG_TIMEOUT = 30
BLOB_WRITE_COMMIT_READ_RETRY = 100

def log(message):
    if DEBUG:
        print str(message)

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
            log(str(self.r) + '\n' + self.hash_before.value + '\n' + str(self.timestamp.value))

class BlobReadWorker(Process):
    ''' Runs GETs against blob being written.  checks against bool.'''
    def __init__(self, obj_name, obj_hash):
        super(BlobReadWorker, self).__init__()
        self.finished_write = False
        self.read_hash = 'No hash yet!'
        self.obj_name = obj_name
        self.write_hash = obj_hash
        log(self.obj_name)

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
                log('Length: ' +  str(sys.getsizeof(r.content)))
                if hashlib.sha1(r.content).hexdigest() == self.write_hash:
                    print 'Success!'
                    return 0
                else:
                    print "Hashes don't match!"

        # If function reaches this point it failed
        print 'Failed!'
        exit(1)

class CommitProbeWorker(threading.Thread):
    def __init__(self, worker_path):
        self.stdout = None
        self.stderr = None
        self.return_code = None
        self.path = worker_path
        threading.Thread.__init__(self)

    def run(self):
        log('Thread started!')
        os.chdir(self.path)
        p = subprocess.Popen('./commit-log-probe',
                shell=True,
                stdout=subprocess.PIPE,
                #stderr=subprocess.PIPE
                )
        self.stdout, self.stderr = p.communicate()
        log('Thread complete!')
        self.return_code = p.returncode

def get_root_path():
    return os.path.abspath(os.path.join(os.path.dirname(os.path.realpath(sys.argv[0])), '..'))

def reset_fds():
    ''' Restarts FDS system.  Assumes we are in test directory!'''
    if REMOTE:
        log('[FDS] Down')
        ret = os.system('./fds-tool.py -f cfg/' + REM_CFG + ' -d')
        log('[FDS] Cleaning')
        ret += os.system('./fds-tool.py -f cfg/' + REM_CFG + ' -c')
        log('[FDS] Up')
        ret += os.system('./fds-tool.py -f cfg/' + REM_CFG + ' -u')
    else:
        ret = os.system('./../tools/fds cleanstart')
        time.sleep(SLEEP)
    return ret

def stop_fds():
    if REMOTE:
        log('[FDS] Down')
        ret = os.system('./fds-tool.py -f cfg/' + REM_CFG + ' -d -c')
    else:
        ret = os.system('./../tools/fds stop')
    return ret

def fill_staging_area():
    ''' Write many big blobs to fill up staging area. Once the staging area
    is full, I/O should fail.'''
    commit_log_size = 1048576

    root_path = get_root_path()
    conf_path = os.path.join(root_path, 'config/etc/')
    log(os.chdir(conf_path))
    log(os.system('git checkout -f platform.conf'))
    log('CWD: ' + str(os.getcwd()))

    # Edit config
    for line in fileinput.input('platform.conf', inplace=1):
        print line.replace(DEFAULT_COMMIT_LOG, 'commit_log_size = ' + str(commit_log_size)).rstrip()
    #print 'commit_log_size = ' + str(commit_log_size)
    log(os.system('cat ' + conf_path + '/platform.conf | grep commit_log'))

    # Navigate back to script's WD and reset FDS (should update with new config)
    os.chdir(os.path.join(root_path, 'tools/'))
    reset_fds()

    # Create a JSon for the dm commit log probe to parse
    os.chdir(os.path.join(root_path, 'unit-test/fds-probe/dataset/'))
    log(os.system('./tvc_json.py -c -n 20000 > commit-log.json'))

    # Bring up the probe
    worker_path = os.path.join(root_path, 'Build/linux-x86_64.debug/tests/')
    print 'Worker path:', worker_path
    worker = CommitProbeWorker(worker_path)
    worker.start()
    time.sleep(SLEEP)

    # Curl the JSon to the probe
    log(str(os.chdir(os.path.join(root_path, 'unit-test/fds-probe/dataset/'))))
    log(str(os.system('curl -X PUT --data @commit-log.json http://localhost:8080/abc')))
    log(str(os.system('rm commit-log.json')))

    # Clean up core files
    rem_path = os.path.join(root_path, 'Build/linux-x86_64.debug/tests/')
    log(rem_path)
    log(str(os.system('rm core.*')))

    # Reset config to original state
    os.chdir(conf_path)
    os.system('git checkout -f platform.conf')

    # Cleanup
    os.chdir(os.path.join(root_path, 'test'))
    worker.join(10)
    probe_retcode = worker.return_code
    assert(probe_retcode != None)
    if probe_retcode != 0:
        return 0
    else:
        return 1

def blob_write_incomplete():
    ''' Steps: Write 1 big blob, kill the write before it completes. Loop.'''
    ## Expected Result: No space should be taken up (memory or disk). Incomplete ##
    # Write 50MB blob
    rand_data_name = DATA_PATH+DATA_NAME+'5'
    session = FuturesSession(max_workers=MAX_CONCURRENCY_PUT)
    requests.put(BASE_URL+BUCKET)

    with open(rand_data_name, 'rb') as rand_data:
        future_put = session.put(BASE_URL+BUCKET+OBJ_NAME, data=rand_data.read(), timeout=.1)

    sleep_timer = .666
    time.sleep(sleep_timer)

    # Read data.  should be length 0 or 404
    r = requests.get(BASE_URL+BUCKET+OBJ_NAME)
    log(r)
    log(r.text)
    if r.status_code == 200:
        log(sys.getsizeof(r.content))
        if sys.getsizeof(r.content) > 0:
            return 1
    else:
        return 0

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
    log('Write done!')

    # PUT is done, GETs should now work
    worker.finished_write = True
    log('[Parent thread] ' + str(worker.finished_write))
    worker.join()
    return 0

def multi_blob_write():
    ''' Writes multiple blobs to same obj id on the server.'''
    log('[Test] MULTI BLOB WRITE')

    # Create 5 random sets of data
    log('Creating random data')
    for i in range(NUM_PROC):
        os.system('dd if=/dev/urandom of='+DATA_PATH+DATA_NAME+str(i+1)+' bs='+str(10*(i+1))+'M count=1')

    # Asynchronously put 5 objects towards the AM
    # Make sure to keep close track of timestamps
    try:
        log('Creating bucket: ' + str(requests.put(BASE_URL+BUCKET)))
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
    log('FDS object hash:' + str(get_hash))

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
    multi_blob_write_result = multi_blob_write()
    if multi_blob_write_result != 0:
        print 'multi_blob_write test failed!'
    reset_fds()
    blob_write_commit_result = blob_write_commit()
    if blob_write_commit_result != 0:
        print 'blob_write_commit test failed!'
    reset_fds()
    blob_write_incomplete_result=0
    for i in range(10):
        blob_write_incomplete_result += blob_write_incomplete()
    if blob_write_incomplete_result != 0:
        print 'blob_write_incomplete test failed!'

    # Don't run the staging test if we're on remote nodes
    if REMOTE == False:
        # No reset needed before this test, it's done within the function
        fill_staging_area_result = fill_staging_area()
        if fill_staging_area_result != 0:
            print 'fill_staging_area test failed!'
    else:
        fill_staging_area_result = 0

    # Quit FDS
    stop_fds()

    final_retcode = multi_blob_write_result + blob_write_commit_result + blob_write_incomplete_result + fill_staging_area_result
    if final_retcode == 0:
        print 'All tests concluded successfully!'
    return final_retcode

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--am_ip', default='127.0.0.1',
                        help='AM IP address')
    parser.add_argument('--config', default='',
                        help='Name of configuration file for remote nodes')
    parser.add_argument('-v', action='store_true', help='Enables verbose output')
    args = parser.parse_args()

    DEBUG = args.v
    REM_CFG = args.config
    BASE_URL = 'http://' + args.am_ip + ':8000/'

    if BASE_URL != 'http://127.0.0.1:8000/':
        REMOTE = True

    main()
