#!/usr/bin/python

import os,re,sys
import json
import time
import dataset
import logging

def write_records(db, series, records):
    cols, vals = [list(x) for x in  zip(*records)]
    table = {}
    table["#timestamp"] = time.strftime("%Y-%m-%d %H:%M:%S", time.gmtime())
    for i, c in enumerate(cols):
        table[c] = vals[i]
    db[series].insert(table)

def main():
    series = sys.argv[1]
    filein = sys.argv[2]
    logging.basicConfig(level=logging.INFO)
    with open(filein, "r") as f:
        records = [ [re.sub(' ','',y) for y in x.split('=')] for x in filter(lambda x : x != "", re.split("[\n;,]+", f.read()))]
    database="experiments"
    connection = "mysql://perf@matteo-vm/" + database
    logging.debug("Connecting: " + connection)
    db = dataset.connect(connection)
    write_records(db, series, records)
    # res = db[series].all()
    # print json.dumps(list(res))

if __name__ == "__main__":
    main()
