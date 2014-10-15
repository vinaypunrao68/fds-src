#!/usr/bin/python
# This script runs threadpool_test while changing params '--cnt', '--iter-cnt' of threadpool_test
# and outputs the result in a tabular form.
# To run this script, you must be run from this directory

import subprocess
import sys
import re
from tabulate import tabulate

def extract_number(s):
    regex=r'[-+]?[0-9]*\.?[0-9]+(?:[eE][-+]?[0-9]+)?'
    return re.findall(regex,s)

cnt = [1000, 5000, 10000, 20000, 40000]
iterations = [1000, 10000, 50000]
results = []

for i in iterations:
    for c in cnt:
        args = ['../../Build/linux-x86_64.debug/tests/threadpool_test',
            '--cnt={}'.format(c),
            '--iter-cnt={}'.format(i),
            '--gtest_filter=ThreadPoolTest.singlethread']
        print args
        output = subprocess.check_output(args)
        print output
        # parse out total time, avg time, avg latency
        output = output.split('\n')
        for line in output:
            if re.search( r'Total time', line):
                total_time = float(extract_number(line)[0])
            elif re.search( r'Avg time', line):
                avg_time = float(extract_number(line)[0])
            elif re.search( r'Avg latency', line):
                avg_latency = float(extract_number(line)[0])
        # append to results
        result = [c, i, total_time, avg_time, avg_latency]
        results.append(result)
        print result

print tabulate(results, ['sched. cnt', 'iter count', 'Total time', 'Avg time', 'Avg latency'])
