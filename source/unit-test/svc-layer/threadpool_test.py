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
"""
iterations = [100000, 500000, 5000000]
cnt = [10000, 40000, 1000000, 10000000, 80000000]
producers = [1, 2, 3, 4]
tests = ['singlethread', 'fdsthreadpool', 'ithreadpool']
results = []
"""
iterations = [100]
cnt = [1000000]
producers = [1, 4]
tests = ['fdsthreadpool', 'ithreadpool']
#tests = ['fdsthreadpool', 'lfsqthreadpool']
results = []

for i in iterations:
    for c in cnt:
        for p in producers:
            stats = []
            for t in tests:
                args = ['perf', 'stat', '../../Build/linux-x86_64.release/tests/threadpool_test',
                    '--type={}'.format(1),
                    '--iter-cnt={}'.format(i),
                    '--cnt={}'.format(c),
                    '--producers={}'.format(p),
                    '--tp-size={}'.format(10),
                    '--gtest_filter=ThreadPoolTest.{}'.format(t)]
                print args
                output = subprocess.check_output(args, stderr=subprocess.STDOUT)
                print output
                # parse out total time, avg time, avg latency
                context_switches = 0
                cpu = 0
                output = output.split('\n')
                for line in output:
                    if re.search( r'Avg latency', line):
                        avg_latency = float(extract_number(line)[0])
                    elif re.search( r'Avg worktime', line):
                        avg_worktime = float(extract_number(line)[0])
                    elif re.search( r'Throughput', line):
                        throughput = float(extract_number(line)[0])
                    elif re.search(r'context-switches', line):
                        line = line.replace(',','')
                        context_switches = int(extract_number(line)[0])
                    elif re.search(r'CPUs utilized', line):
                        cpu = float(extract_number(line)[1])
                # append stats
                stats.extend([avg_latency, avg_worktime, throughput, context_switches, cpu])
            result = [p, c, i]
            #result = [p, c]
            result.extend(stats)
            results.append(result)
            print result

print tabulate(results, ['prdcrs', 'schdCnt', 'itr',
        'S AvgLat', 'wktime', 'throughput', 'cs', 'cpu',
        'F AvgLat', 'wktime', 'throughput', 'cs', 'cpu',
        'C AvgLat', 'wktime', 'throughput', 'cs', 'cpu'])
