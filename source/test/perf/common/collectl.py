#!/usr/bin/python
import os, re, sys
import subprocess
import socket
from optparse import OptionParser
import signal
from push_to_influxdb import InfluxDb
import tabulate
import threading

class SystemMonitor(object):
    def __init__(self, config):
        self.config  = config
        self.influxdb = InfluxDb(config["influxdb_config"], False)
        self.stop = threading.Event()
        self.ps = subprocess.Popen(('collectl', '-P', '-m'), stdout=subprocess.PIPE)
        self.hostname = socket.gethostname()

    def run(self):
        line = self.ps.stdout.readline()
        line = self.ps.stdout.readline()
        labels = line.rstrip("\n").split()
        while line: 
            line = self.ps.stdout.readline()
            tokens = line.rstrip("\n").split()
            records = []
            for i, v in enumerate(tokens):
                records.append((labels[i], v))
                #self.print_records(records)
                series = "system_stats." + self.hostname
                self.influxdb.write_records(series, records)
        
    def terminate(self):
        self.stop.set()
        self.ps.kill()

    def print_records(self, recs):
        print tabulate.tabulate(recs)



def main():
    parser = OptionParser()
    parser.add_option("-p", "--period", dest = "period", type = "float", default = 1.0,
                      help = "Period -> NOt implemented yet")

    (options, args) = parser.parse_args()

    influx_db_config = {
        "ip" : "matteo-vm",
        "port" : 8086,
        "user" : "root",
        "password" : "root",
        "db" : "system_stats"
    }
    
    config = {
        "influxdb_config" : influx_db_config,
        "period" : options.period,
    }
    print "Config:", config
    m =  SystemMonitor(config)
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
