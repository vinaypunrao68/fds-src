#!/usr/bin/python

import os,re,sys
import json
import time
import dataset

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

def write_records(db, records):
    cols, vals = [list(x) for x in  zip(*records)]
    vals = [dyn_cast(x) for x in vals]
    table = {}
    table["#timestamp"] = time.strftime("%Y-%m-%d %H:%M:%S", time.gmtime())
    for i, c in enumerate(cols):
        table[c] = vals[i]
    db["experiments"].insert(table)

def main():
    series = sys.argv[1]
    filein = sys.argv[2]
    config = {"dbfile" : "/regress/%s.db" % series}
    with open(filein, "r") as f:
        records = [ [re.sub(' ','',y) for y in x.split('=')] for x in filter(lambda x : x != "", re.split("[\n;,]+", f.read()))]
    print 'sqlite:///%s' % config["dbfile"]    
    db = dataset.connect('sqlite:///%s' % config["dbfile"])
    write_records(db, records)
    res = db["experiments"].all()
    print json.dumps(list(res))

if __name__ == "__main__":
    main()
