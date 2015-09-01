#!/usr/bin/python
import threading
import sys
import subprocess
import copy

def task_runner(task, sem):
    subprocess.call(task, shell=True)
    sem.release()
    

def run_n(tasks, concurrency):
    sem = threading.Semaphore(concurrency)
    threads = []
    while len(tasks) > 0:
        task = tasks.pop()
        sem.acquire()
        t = threading.Thread(target=task_runner, args=(task, sem))
        t.start()
        threads.append(t)
    for t in threads:
        t.join()

cmd_file = sys.argv[1]
concurrency = int(sys.argv[2])

with open(cmd_file, "r") as f:
    cmds = [x.rstrip('\n') for x in list(f)]
run_n(cmds, concurrency)



