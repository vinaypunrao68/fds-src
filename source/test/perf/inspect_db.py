#!/usr/bin/python

import os,re,sys
import dataset
import tabulate


if __name__ == "__main__":
    test_db = sys.argv[1]
    db = dataset.connect('sqlite:///%s' % test_db)
    experiments = db["experiments"].all()
    experiments = [ x for x in experiments]
    tags = set()
    [tags.add(x) for x in [x["tag"] for x in experiments]]
    for e in experiments:
        print "---------"
        for k,v in e.iteritems():
           print "\t",k,v
    print "tags:", list(tags)

