#!/usr/bin/python
import os, sys, re
from exp_framework import *
import json
from tests import TestList
from optparse import OptionParser
import  perf_framework_utils as utils

# FIXME: import only what you use
# TODO: better configuration
# TODO: need to streamline the options

def main():
    parser = OptionParser()

    # Pyro options
    parser.add_option("-n", "--name-server-ip", dest = "ns_ip", default = "10.1.10.222", help = "IP of the name server")
    parser.add_option("-p", "--name-server-port", dest = "ns_port", type = "int", default = 47672, help = "Port of the name server")
    parser.add_option("", "--force-cmd-server", dest = "force_cmd_server", action = "store_true", default = False,
                      help = "Force start of command server")

    parser.add_option("-v", "--log-severity", dest = "log_severity", default = "INFO", help = "Log severity level: DEBUG, INFO, WARNING, ERROR, CRITICAL")

    # Main FDS cluster configuration
    parser.add_option("-m", "--main-node", dest = "main_node", default = "luke", help = "Node AM is on")

    # Test configuration
    #FIXME: rename: test_node is an IP
    parser.add_option("-t", "--test-node", dest = "test_node", default = None,
                      help = "IP of the node the test node will run from. Default will run from local")

    parser.add_option("-J", "--json-file", dest = "json_file",
                      default = None,
                      help = "Read from json")

    # Global experiment-level
    parser.add_option("-d", "--experiment-directory", dest = "experiment_directory", default = "experiment",
                      help = "Experiment directory (default is <experiment>)")

    # FdsCluster
    parser.add_option("-x", "--no-fds-start", dest = "no_fds_start", action = "store_true", default = False,
                      help = "Don't start FDS")

    # CounterServer
    parser.add_option("-c", "--counter-pull-rate", dest = "counter_pull_rate", type = "float", default = 5.0,
                      help = "Counters pull rate")

    parser.add_option("-j", "--java-counters", dest = "java_counters", action = "store_true", default = False,
                      help = "Pull java counters")
    parser.add_option("-D", "--fds-directory", dest = "fds_directory", default = "/home/monchier/regress/fds-src",
                      help = "FDS root directory")
    parser.add_option("", "--fds-nodes", dest = "fds_nodes", default = "han",
                      help = "List of FDS nodes (for monitoring)")

    (options, args) = parser.parse_args()

    # Hardcoded options

    #logging.basicConfig( stream=sys.stderr, level=logging.INFO )
    #options.logger = logging.getLogger("perf_exp_framework")
    options.logger = utils.PerfLogger("perf_exp_framework", utils.PerfLogger.get_level_from_str(options.log_severity))

    # FDS nodes
    options.nodes = {}
    for n in options.fds_nodes.split(","):
        options.nodes[n] = utils.get_ip(n)
    options.logger.info("FDS nodes: " + str(options.nodes))
    options.myip = utils.get_my_ip()
    options.logger.info("My IP:" + options.myip)

    options.logger.info( "Options: " + str(options))

    tl = TestList()
    if options.json_file == None:
        tl.create_tests()
        tests = tl.get_tests()
    else:
        with open(options.json_file, "r") as _f:
            tests = json.load(_f)
    assert tests != None, "Tests not loaded"

    options.logger.info("--- Number of tests to run " + str(len(tests)) + " ---")
    for t in tests:
        options.logger.info("test --> " + str(t))
    options.logger.info("--- . ---")

    if not os.path.exists(options.experiment_directory):
        os.makedirs(options.experiment_directory)
    os.chdir(options.experiment_directory)

    # start FDS
    fds = FdsCluster(options)
    fds.restart()

    fds.init_test_once(tests[1]["test_type"], tests[0]["nvols"])  # FIXME: this is ugly... 

    time.sleep(10)

    # Execute tests
    for t in tests:
        options.logger.info(t)
        results_dir = utils.get_results_dir("results", t)
        fds.init_test(t["test_type"], results_dir, t)
        # FIXME: need to reset counters!
        # start stats collection

        time.sleep(10)

        counter_server = CounterServerPull(results_dir, options)
        monitors = Monitors(results_dir, options)
        monitors.compute_monitors_cmds(fds.get_pidmap())
        monitors.run()

        if t["injector"] != None:
            injector = CommandInjector(options, t["injector"])
            injector.start()

        time.sleep(10)

        # execute test
        # time.sleep(5)
        fds.run_test(t["test_type"], t)
    
        # terminate stats collection
        monitors.terminate()
        counter_server.terminate()
        if t["injector"] != None:
            injector.terminate()
        time.sleep(5)
    # terminate FDS
    # fds.terminate()

if __name__ == "__main__":
    main()
