#!/usr/bin/python

import sqlite3
import os, sys
import tabulate
import optparse

parser = optparse.OptionParser("usage: %prog [options]")
def dumpTimeline():
    filename = "{}/sys-repo/dm-names/timeline.db".format(args.root)

    if not os.path.exists(filename):
        print 'db does not exist :', filename
        return

    conn = sqlite3.connect(filename)
    snapshotcols = []
    for row in conn.execute("PRAGMA table_info('snapshottbl')"):
        snapshotcols.append(row[1])

    journalcols = []
    for row in conn.execute("PRAGMA table_info('journaltbl')"):
        journalcols.append(row[1])

    data=[]
    for row in conn.execute("select * from journaltbl"):
        data.append(row)

    print 'journal table :' , len(data)
    print tabulate.tabulate(data, headers= journalcols)

    data = []
    for row in conn.execute("select * from snapshottbl"):
        r = (row[0], (int)(row[1]/1000000) + 1, row[2])
        data.append(r)
    print 'snapshot table :', len(data)
    print tabulate.tabulate(data, headers= snapshotcols)

def dumpActiveObjects():
    filename = "{}/user-repo/liveobj.db".format(args.root)

    if not os.path.exists(filename):
        print 'db does not exist :', filename
        return

    conn = sqlite3.connect(filename)
    cols = []
    for row in conn.execute("PRAGMA table_info('liveobjectstbl')"):
        cols.append(row[1])

    data=[]
    for row in conn.execute("select * from liveobjectstbl where length(filename)>0 order by smtoken,volid"):
        data.append(row)

    print 'active objects table :' , len(data)
    print tabulate.tabulate(data, headers=cols)


if  __name__ == '__main__':
    parser.add_option('-f','--fds-root',dest='root',default='/fds', help = 'fds root dir')
    parser.add_option('-t','--timeline',action = 'store_true', default=False, help = 'dump timeline db')
    parser.add_option('-a','--active'  , action = 'store_true', default=False, help = 'dump active objects db')

    (args, other) = parser.parse_args()

    if not (args.active or args.timeline):
        args.active = True

    if args.active:
        dumpActiveObjects()

    if args.timeline:
        dumpTimeline()
