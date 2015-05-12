#!/usr/bin/python

import sqlite3
import os, sys
import tabulate

filename = "/fds/sys-repo/dm-names/timeline.db"

if not os.path.exists(filename):
    print 'db does not exist :', filename
    sys.exit(0)

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

