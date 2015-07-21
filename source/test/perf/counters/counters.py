#!/usr/bin/python

import os, re, sys
import time
from optparse import OptionParser
import signal
import threading
sys.path.append(os.path.join(os.getcwd(), '../common'))
from push_to_influxdb import InfluxDb
import tabulate

sys.path.append(os.path.join(os.getcwd(), '../../fdslib'))
sys.path.append(os.path.join(os.getcwd(), '../../fdslib/pyfdsp'))
from SvcHandle import SvcMap

class CounterMonitor(object):
    def __init__(self, config):
        self.config  = config
        self.influxdb = InfluxDb(config["influxdb_config"], False)
        self.svc_map = SvcMap(self.config["ip"], self.config["port"])
        self.stop = threading.Event()

    def get_svc_table(self):    
        table = []
        for e in self.svc_map.list():
            table.append({
                    "uuid" : e[0],
                    "name" : e[1],
                    "ip"   : e[3],
                    "port" : e[4],
                    "status":e[5],
                })
        for e in table:
            for k, v in e.iteritems():
                print k + ":" + str(v),
            print ""
        return table
    
    def run(self):
        while not self.stop.isSet():
            time.sleep(self.config["period"])
            self.get_and_process_counters()

    def get_and_process_counters(self):
        def check_agent_filter(a):
            if not self.config["agent_filter"]:
                return True
            for e in self.config["agent_filter"]:
                if a == e:
                    return True
            return False
        def check_ip_filter(ip):
            if not self.config["ip_filter"]:
                return True
            for e in self.config["ip_filter"]:
                if ip == e:
                    return True
            return False
        self.svc_table = self.get_svc_table()
        for e in self.svc_table:
            if (    e["status"] == "Active" and 
                    check_agent_filter(e["name"]) and 
                    check_ip_filter(e["ip"])
                    ):
                try:
                    cntr = self.svc_map.client(e["uuid"]).getCounters('*')
                except:
                    cntr = {}
                timestamp = time.time()
        
                series = e["name"] + "." + e["ip"]
                records = []
                for k, v in e.iteritems():
                    records.append((k, v))
                for k,v in cntr.iteritems():
                    if self.config["cntr_filter"]:
                        for e in self.config["cntr_filter"]:
                            if e in k:
                                records.append((k, v))
                    else:
                        records.append((k, v))
                records.append(("time", timestamp))
                self.print_records(records)
                self.influxdb.write_records(series, records)

    def terminate(self):
        self.stop.set()

    def print_records(self, recs):
        print tabulate.tabulate(recs)

def main():
    parser = OptionParser()
    parser.add_option("-H", "--host-ip", dest = "host_ip", default = "luke", help = "Host IP")
    parser.add_option("-P", "--host-port", dest = "host_port", type = "int", default = 7020,
                      help = "Host port")
    parser.add_option("-p", "--period", dest = "period", type = "float", default = 1.0,
                      help = "Counter period")
    parser.add_option("-f", "--counter-filter", dest = "counter_filter", default = None,
                      help = "Filter counters based on name")
    parser.add_option("", "--ip-filter", dest = "ip_filter", default = None,
                      help = "Filter counters based on the ip")
    parser.add_option("", "--agent-filter", dest = "agent_filter", default = None,
                      help = "Filter counters based on the agent")

    (options, args) = parser.parse_args()

    influx_db_config = {
        "ip" : "matteo-vm",
        "port" : 8086,
        "user" : "root",
        "password" : "root",
        "db" : "counters"
    }
    
    config = {
        "influxdb_config" : influx_db_config,
        "ip" : options.host_ip,
        "port" : options.host_port,
        "period" : options.period,
        "cntr_filter" : options.counter_filter.split(',') if options.counter_filter else None,
        "agent_filter" : options.agent_filter.split(',') if options.agent_filter else None,
        "ip_filter" : options.ip_filter.split(',') if options.ip_filter else None,
    }
    print "Config:", config
    m =  CounterMonitor(config)
    t = threading.Thread(target = m.run)
    t.start()

    def signal_handler(signal, frame):
        print('... Exiting. Wait a few seconds...')
        m.terminate()
        sys.exit(0)

    signal.signal(signal.SIGINT, signal_handler)
    print('Press Ctrl+C to exit.')
    signal.pause()



if __name__ == "__main__":
    main()
