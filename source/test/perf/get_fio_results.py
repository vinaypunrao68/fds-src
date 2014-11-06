#!/usr/bin/python

import os,re,sys

# rand_writes.sdg.4k.128.16.fio

dir = sys.argv[1]

files = os.listdir(dir)

for fn in files:
    with open(dir + "/" + fn, "r") as f:
        test, disk, bs, ag, jobs, _ =  fn.split(".")

        rbw = 0.0
        riops = 0.0
        rclat = 0.0
        wbw = 0.0
        wiops = 0.0
        wclat = 0.0
        cntr = 0
        for l in f:
            tokens = l.rstrip("\n").split(";")
            rbw += float(tokens[6])
            riops += float(tokens[7])
            rclat += float(tokens[15])
            wbw += float(tokens[47])
            wiops += float(tokens[48])
            wclat += float(tokens[55])
            cntr += 1
        output = [test, disk, bs, ag, jobs, rbw, riops, rclat/cntr, wbw, wiops, wclat/cntr]
        #output = [str(x) + "," for x in output]
        output = reduce(lambda x,y : str(x) + ", " + str(y), output)
        sys.stdout.write(output + "\n")

