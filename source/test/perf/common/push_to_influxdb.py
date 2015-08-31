#!/usr/bin/python

import os,re,sys
import json
import logging
import logging.handlers
from influxdb import client as influxdb
from optparse import OptionParser
sys.path.append("../common")
from utils import *

class InfluxDb:
    def __init__(self, config, logging_enabled):
        self.ip = config["ip"]
        self.port = config["port"]
        self.user = config["user"]
        self.password = config["password"]
        self.dbname = config["db"]
        self.db = influxdb.InfluxDBClient(self.ip, self.port, self.user, self.password, self.dbname)
        self.logging_enabled = logging_enabled

    def write_records(self, series, records):
        cols, vals = [list(x) for x in  zip(*records)]
        vals = [dyn_cast(x) for x in vals]
        data = [
          {
           "name" : series,
            "columns" : cols,
            "points" : [
              vals 
            ]
          }
        ]
        self.db.write_points(data)
        if self.logging_enabled:
            logger.info((json.dumps(data)))

influx_db_config = {
    "ip" : "matteo-vm",
    "port" : 8086,
    "user" : "root",
    "password" : "root",
    "db" : "perf",
}


if __name__ == "__main__":
    parser = OptionParser()
    parser.add_option("", "--influxdb-host", dest = "influxdb_host", default = "matteo-vm",
                      help = "Influxdb host")
    parser.add_option("", "--influxdb-port", dest = "influxdb_port", default = 8086, type = "int",
                      help = "Influxdb port")
    parser.add_option("", "--influxdb-db", dest = "influxdb_db", default = "perf",
                      help = "Influxdb db")
    parser.add_option("", "--influxdb-user", dest = "influxdb_user", default = "root",
                      help = "Influxdb user")
    parser.add_option("", "--influxdb-password", dest = "influxdb_password", default = "root",
                      help = "Influxdb password")
    (options, args) = parser.parse_args()
    
    influx_db_config = {
        "ip" : options.influxdb_host,
        "port" :  options.influxdb_port,
        "user" :  options.influxdb_user,
        "password" :  options.influxdb_password,
        "db" :  options.influxdb_db
    }

    series = args[0]
    filein = args[1]
    print options, args
    logfile = "/regress/log.log.bz2"
    assert os.path.exists(os.path.dirname(logfile)), "Directory dos not exist: " + os.path.dirname(logfile)
    FORMAT = '%(asctime)-15s PerfLog %(message)s'
    logger = logging.getLogger(__name__)
    logger.setLevel(logging.DEBUG)
    handler = logging.handlers.RotatingFileHandler(
        logfile, maxBytes=100*1024*1024, backupCount=500, encoding='bz2-codec')
    formatter = logging.Formatter(FORMAT)
    handler.setFormatter(formatter)
    logger.addHandler(handler)

    with open(filein, "r") as f:
        records = [ [re.sub(' ','',y) for y in x.split('=')] for x in filter(lambda x : x != "", re.split("[\n;,]+", f.read()))]
            
    print "InfluxDB Config:", influx_db_config
    db = InfluxDb(influx_db_config, True)
    db.write_records(series, records)

