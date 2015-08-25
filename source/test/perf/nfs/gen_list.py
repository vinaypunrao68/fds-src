#!/usr/bin/python

import sys
import random
import os

filein = sys.argv[1] 
fileout = sys.argv[2]
n = int(sys.argv[3])

with open(filein, "r") as f:
    files = [x.rstrip('\n') for x in list(f)]

out = []
for i in range(n):
    out.append(random.choice(files))

with open(fileout, "w") as f:
    for e in out:
        f.write(e+"\n")

