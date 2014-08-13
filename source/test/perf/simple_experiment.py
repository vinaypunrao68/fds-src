#!/usr/bin/python
import os, sys, re
from exp_framework import *

# FIXME: import only what you use
# TODO: better configuration
# FdsCluster, get_results_dir (maybe move this), CuonterServer, Monitor, nodes (configuration!)

def create_tests():
    # test template
    template = {
        "test_type" : "tgen",
        "nvols" : 4,
        "threads" : 1,
        "nreqs" : 10000,
        "type" : "PUT",
        "fsize" : 4096,
        "nfiles" : 1000
        }

    ############### Test definition ############

    tests = []
    size = 4096   # 4096
    test = dict(template)
    test["type"] = "PUT"
    test["nreqs"] =  10000  # 100000
    test["nfiles"] = 10000 # 10000
    test["nvols"] = 2
    test["threads"] = 1
    test["fsize"] = size
    # tests.append(test)
    # for nvols in [1, 2, 3, 4]:
    #     for th in [1, 2, 5, 10, 15]:
    #         test = dict(template)
    #         test["type"] = "PUT"
    #         test["nreqs"] = 10000
    #         test["nfiles"] = 10000
    #         test["nvols"] = nvols
    #         test["threads"] = th
    #         tests.append(test)
    for nvols in [1]:#[1, 2]: # [1, 2, 3, 4]:
        for th in [15]:#[1, 5, 10, 15, 20, 25, 30]: #[1, 2, 5, 10, 15]:
            test = dict(template)
            test["type"] = "GET"
            test["nreqs"] = 10000 # 100000
            test["nfiles"] = 10000 # 10000
            test["nvols"] = nvols
            test["threads"] = th
            test["fsize"] = size
            tests.append(test)
    size = 4096 * 1024
    test = dict(template)
    test["type"] = "PUT"
    test["nreqs"] = 10000
    test["nfiles"] = 100
    test["nvols"] = 2
    test["threads"] = 1
    test["fsize"] = size
    #tests.append(test)
    for nvols in [1, 2]:
        for th in [1, 2, 5, 10, 15, 20, 25, 30]:
            test = dict(template)
            test["type"] = "GET"
            test["nreqs"] = 10000
            test["nfiles"] = 100
            test["nvols"] = nvols
            test["threads"] = th
            test["fsize"] = size
            #tests.append(test)

    return tests

def main():
    parser = OptionParser()
    #parser.add_option("-d", "--directory", dest = "directory", help = "Directory")
    #parser.add_option("-d", "--node-name", dest = "node_name", default = "node", help = "Node name")
    parser.add_option("-n", "--name-server-ip", dest = "ns_ip", default = "10.1.10.102", help = "IP of the name server")
    parser.add_option("-p", "--name-server-port", dest = "ns_port", type = "int", default = 47672, help = "Port of the name server")
    parser.add_option("-l", "--local", dest = "local", action = "store_true", default = True, help = "Run locally")
    parser.add_option("-r", "--remote-test-node", dest = "test_node", default = None, help = "Remote test node")
    parser.add_option("-L", "--local-node", dest = "local_node", default = "localhost", help = "Local node (default is localhost)")
    parser.add_option("-d", "--experiment-directory", dest = "experiment_directory", default = "experiment", help = "Experiment directory (default is <experiment>)")
    parser.add_option("-x", "--no-fds-start", dest = "no_fds_start", action = "store_true", default = False, help = "Don't start FDS")
    (options, args) = parser.parse_args()
    options.remote_fds_root =  "/home/monchier/FDS/dev"
    options.local_fds_root =  "/home/monchier/FDS/dev"

    if not os.path.exists(options.experiment_directory):
        os.makedirs(options.experiment_directory)
    os.chdir(options.experiment_directory)

    # start FDS

    tests = create_tests()
    fds = FdsCluster("tiefighter", options)
    fds.start()

    print "--- Number of tests to run", len(tests), "---"
    for t in tests:
        print "test -->", t
    print "--- . ---"

    for t in tests:
        print t
        results_dir = get_results_dir("results", t)
        fds.init_test(t["test_type"], results_dir, t)
        # FIXME: need to reset counters!
        # start stats collection
        counter_server = CounterServer(results_dir)
        #cmds = ["iostat -d 1", "collectl"]
        monitors = Monitors(None, results_dir, nodes, options)
        monitors.compute_monitors_cmds(fds.get_pidmap())
        monitors.run()
    
        # execute test
        # time.sleep(5)
        fds.run_test(t["test_type"], t)
    
        # terminate stats collection
        monitors.terminate()
        counter_server.terminate()

        time.sleep(5)
    # terminate FDS
    # fds.terminate()

if __name__ == "__main__":
    main()
