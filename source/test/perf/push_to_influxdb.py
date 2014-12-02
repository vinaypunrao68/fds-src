#!/usr/bin/python

import os,re,sys
import tabulate
import dataset
import operator 
from influxdb import client as influxdb
import random
import datetime

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

class UniqueId(object):
    def __init__(self, init=0):
        self.reset(init)

    def reset(self, init):
        self.id = init

    def get(self):
        val = self.id
        self.id +=1
        return val

    def dump(self):
        return self.id

class InfluxDb:
    def __init__(self, config, uid_gen):
        self.ip = config["ip"]
        self.port = config["port"]
        self.user = config["user"]
        self.password = config["password"]
        self.dbname = "perf"
        self.uid_gen = uid_gen

    def write_to_influxdb(self, records):
        db = influxdb.InfluxDBClient(self.ip, self.port, self.user, self.password, self.dbname)
        uid = self.uid_gen.get()
        date = str(datetime.date.today())
        for e in records:
            cols, vals = [list(x) for x in  zip(*e.iteritems())]
            vals = [dyn_cast(x) for x in vals]
            tag = re.sub(':','-',vals[cols.index("tag")])
            # print "Adding to", tag, "->",vals
            cols.append("date")
            cols.append("uid")
            vals.append(date)
            vals.append(uid)
            data = [
              {
               "name" : tag,
                "columns" : cols,
                "points" : [
                  vals 
                ]
              }
            ]
            db.write_points(data)

influx_db_config = {
    "ip" : "metrics.formationds.com",
    "port" : 8086,
    "user" : "root",
    "password" : "root",
}

if __name__ == "__main__":
    test_db = sys.argv[1]
    print "InfluxDB Config:", influx_db_config
    init_uid = 0
    if os.path.exists('/regress/uid'):
        with open('/regress/uid', 'r') as f:
            try:
                init_uid = int(f.read())
            except:
                init_uid = 0
    uid_gen = UniqueId(init_uid)
    influx_db = InfluxDb(influx_db_config, uid_gen)
     
    db = dataset.connect('sqlite:///%s' % test_db)
    experiments = db["experiments"].all()
    experiments = list(experiments)
    influx_db.write_to_influxdb(experiments)
    with open('/regress/uid', 'w') as f:
        f.write(str(uid_gen.dump()))

