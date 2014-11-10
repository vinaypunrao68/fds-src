#!/usr/bin/python

import os, re, sys, time
from exp_framework import CounterServerPull
import signal
from optparse import OptionParser

# class Options():
#     def __init__(self):
#         self.counter_pull_rate = 1 
#         self.java_counters = False
#         self.nodes = {"luke" : "10.1.10.222"}
#         self.main_node = "luke"

if __name__ == "__main__":
    parser = OptionParser()
    parser.add_option("-o", "--output", dest = "output", help = "Output file prefix")
    parser.add_option("-n", "--node", dest = "main_node", default = "luke", help = "Node AM is on")
    # parser.add_option("", "--fds-nodes", dest = "fds_nodes", default = "han",
    #                  help = "List of FDS nodes (for monitoring)")
    parser.add_option("-c", "--counter-pull-rate", dest = "counter_pull_rate", type = "float", default = 5.0,
                      help = "Counters pull rate")

    parser.add_option("-j", "--java-counters", dest = "java_counters", action = "store_true", default = False,
                      help = "Pull java counters")
 
    (opts, args) = parser.parse_args()
    NODES = {
        "node1" : "10.1.10.16",
        "node2" : "10.1.10.17",
        "node3" : "10.1.10.18",
        "tiefighter" : "10.1.10.102",
        "luke" : "10.1.10.222",
        "han" : "10.1.10.139",
        "chewie" : "10.1.10.80",
        "c3po" : "10.1.10.221",
    }
    opts.nodes = {opts.main_node : NODES[opts.main_node]}

    # options = Options()
    server = CounterServerPull("./", opts, opt_outfile=opts.output)

    def signal_handler(signal, frame):
        print('... Exiting. Wait a few seconds...')
        server.terminate()
        sys.exit(0)

    signal.signal(signal.SIGINT, signal_handler)
    print('Press Ctrl+C to exit.')
    signal.pause()



