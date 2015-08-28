#!/usr/bin/python
import subprocess
import logging
import time
import threading
import sys
import os
from optparse import OptionParser
import signal
sys.path.append(os.path.join(os.getcwd(), '../common'))
from push_to_influxdb import InfluxDb

agents={ "sm" : "StorMgr", "dm" : "DataMgr", "am" : "bare_am", "xdi" : "\.am\.Main", "om" : "\.om\.Main"}

def run(period, influxdb, stop, host_ip):
    while not stop.isSet():
        for a, v in agents.iteritems():
            try:
                output = subprocess.check_output("ps aux| grep %s | grep -v grep" % v, shell=True).rstrip("\n")
            except:
                continue
            logging.info(output)
            tokens = output.split()
            cpu = tokens[2]
            mem = tokens[3]
            records = [("cpu", cpu), ("mem", mem)]
            series = host_ip + "." + a
            influxdb.write_records(series, records)
        time.sleep(period)

def main():
    parser = OptionParser()
    parser.add_option("-H", "--host-ip", dest = "host_ip", default = "luke", help = "Host IP")
    parser.add_option("-p", "--period", dest = "period", type = "float", default = 1.0,
                      help = "Counter period")
    parser.add_option("", "--influxdb-host", dest = "influxdb_host", default = "han.formationds.com",
                      help = "Influxdb host")
    parser.add_option("", "--influxdb-port", dest = "influxdb_port", default = 8086, type = "int",
                      help = "Influxdb port")
    parser.add_option("", "--influxdb-db", dest = "influxdb_db", default = "breakdown_stats",
                      help = "Influxdb db")
    parser.add_option("", "--influxdb-user", dest = "influxdb_user", default = "root",
                      help = "Influxdb user")
    parser.add_option("", "--influxdb-password", dest = "influxdb_password", default = "root",
                      help = "Influxdb password")

    (options, args) = parser.parse_args()
    logging.basicConfig(level=logging.INFO)

    influx_db_config = {
        "ip" : options.influxdb_host,
        "port" :  options.influxdb_port,
        "user" :  options.influxdb_user,
        "password" :  options.influxdb_password,
        "db" :  options.influxdb_db
    }
    
    config = {
        "influxdb_config" : influx_db_config,
    }
    print "Config:", config
    influxdb = InfluxDb(config["influxdb_config"], False)
    stop = threading.Event()
    t = threading.Thread(target = run, args=(options.period, influxdb, stop, options.host_ip))
    t.start()

    def signal_handler(signal, frame):
        print('... Exiting. Wait a few seconds...')
        stop.set()
        sys.exit(0)

    signal.signal(signal.SIGINT, signal_handler)
    print('Press Ctrl+C to exit.')
    signal.pause()

if __name__ == "__main__":
    main()
