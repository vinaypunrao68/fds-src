#!/usr/bin/python

import os,re,sys
import tabulate
import operator
import dataset
import copy
import pickle


def order_by(table, columns):
	column, columns = columns[0], columns[1:]
	print column, columns
	if len(columns) > 0:
		table = order_by(table, columns)
	table = sorted(table, key = lambda x : x[column])
	return table

directory=sys.argv[1]

# connect to temporary db
db = dataset.connect('sqlite:////tmp/test.db')

files = os.listdir(directory)
dictionary = {}

prev_key = None

for f in files:
    print f
    # out.n_reqs=1000000.n_file=1000.outstanding_reqs=100.test_type=GET.object_size=4096.hostname=luke.n_conns=100.job=4
    m = re.match("out\.n_reqs=(\d+)\.n_files=(\d+)\.outstanding_reqs=(\d+)\.test_type=(\w+)\.object_size=(\d+)\.hostname=(\w+)\.n_conns=(\d+)\.job=(\d+)",f)
    assert m is not None
    params = {
        "n_reqs" : int(m.group(1)),
        "n_files" : int(m.group(2)),
        "outstanding_reqs" : int(m.group(3)),
        "test_type" : m.group(4),
        "object_size" : int(m.group(5)),
        "hostname" : m.group(6),
        "n_conns" : int(m.group(7)),
    }
    n_jobs = int(m.group(8))

    key = pickle.dumps(params)
    if key != prev_key:
        iops = 0.0
    with open(directory + "/" + f, "r") as _f:
        contents = _f.read()
    for l in contents.split("\n"):
        m = re.search("IOPs:\s*(\d+)",l)
        if m is not None:
            iops += float(m.group(1))

    dictionary[key] = {"n_jobs" : n_jobs+1, "iops" : iops}

table = []
for k,v in dictionary.items():
    table_item = pickle.loads(k)
    table_item.update(v)
    table.append(table_item)


for e in table:
    db["experiments"].insert(e)

#table = sorted(table, key = lambda x : x["workload"])
cols = ["n_jobs", "outstanding_reqs", "test_type", "object_size", "n_files"]
table = order_by(table, cols)
headers = cols + ["iops"]
print tabulate.tabulate([[x["n_jobs"], x["outstanding_reqs"], x["test_type"], x["object_size"], x["n_files"], x["iops"]] for x in table], headers)
