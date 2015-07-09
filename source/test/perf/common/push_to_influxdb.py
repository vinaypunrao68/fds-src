#!/usr/bin/python

import os,re,sys
import json
import logging
import logging.handlers
from influxdb import client as influxdb

def is_float(x):
    try:
        float(x)
    except (ValueError, TypeError):
        return False
    else:
        return True

def is_int(x):
    try:
        int(x)
    except (ValueError, TypeError):
        return False
    else:
        return True

def dyn_cast(val):
    if is_float(val):
        return float(val)
    elif is_int(val):
        return int(val)
    else:
        return val

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
        print data
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
    series = sys.argv[1]
    filein = sys.argv[2]
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

