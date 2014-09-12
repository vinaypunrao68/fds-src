#!/usr/bin/python

import os, re, sys, time
from exp_framework import CounterServerPull
import signal


class Options():
    def __init__(self):
        self.counter_pull_rate = 5 
        self.java_counters = False
        self.nodes = {"han" : "10.1.10.139"}
        self.main_node = "han"

if __name__ == "__main__":
    options = Options()
    server = CounterServerPull("./", options)

    def signal_handler(signal, frame):
        print('... Exiting. Wait a few seconds for files to be coped over')
        server.terminate()
        sys.exit(0)

    signal.signal(signal.SIGINT, signal_handler)
    print('Press Ctrl+C to exit.')
    signal.pause()



