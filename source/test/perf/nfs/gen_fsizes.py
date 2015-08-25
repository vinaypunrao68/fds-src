#!/usr/bin/python
import sys
import random

min_1=int(sys.argv[1])
max_1=int(sys.argv[2])
min_2=int(sys.argv[3])
max_2=int(sys.argv[4])
n=int(sys.argv[5])
f=float(sys.argv[6]) # fraction of big files

smalls = [random.randint(min_1, max_1) for x in range(int(n*(1-f)))]
bigs = [random.randint(min_2, max_2)*1024 for x in range(int(n*f))]

for x in smalls+bigs:
    print x,


