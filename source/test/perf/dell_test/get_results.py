#!/usr/bin/python

import os,re,sys
import tabulate
import operator


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
table = []
for f in files:
	# out.numjobs=4.workload=write.bs=512.iodepth=256.disksize=8g
	m = re.match("out.numjobs=(\d+).workload=(\w+).bs=(\d+).iodepth=(\d+).disksize=(\w+)",f)
	assert m is not None
	numjobs = int(m.group(1))
	workload = m.group(2)
	bs = int(m.group(3))
	iodepth = int(m.group(4))
	disksize = m.group(5)
	with open(directory + "/" + f, "r") as _f:
		contents = _f.read()
	iops = 0
	for l in contents.split("\n"):
		m = re.search("iops=(\d+)",l)
		if m is not None:
			iops += int(m.group(1))
	table.append({"numjobs" : numjobs, "workload" : workload, "bs" : bs, "iodepth" : iodepth, "disksize" : disksize, "iops" : iops})
	
	#if workload == "read" or workload == "randread":

# add table to temporary db
db["experiments"].insert(table)

#table = sorted(table, key = lambda x : x["workload"])
table = order_by(table, ["numjobs", "workload", "bs", "iodepth", "disksize"])
headers = {"numjobs" : "numjobs", "workload" : "workload", "bs" : "bs", "iodepth" : "iodepth", "disksize" : "disksize", "iops" : "iops"}
print tabulate.tabulate([[x["numjobs"], x["workload"], x["bs"], x["iodepth"], x["disksize"], x["iops"]] for x in table], [headers["numjobs"], headers["workload"], headers["bs"], headers["iodepth"], headers["disksize"], headers["iops"]])
