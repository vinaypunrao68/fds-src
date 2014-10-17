#!/usr/bin/python
# This script runs serializer_gtest while changing params '--buf-size', '--cnt' of serializer_gtest
# and outputs the result in a tabular form.
# To run this script, you must run from this directory

import subprocess
import sys
import re
from tabulate import tabulate

def extract_number(s):
    regex=r'[-+]?[0-9]*\.?[0-9]+(?:[eE][-+]?[0-9]+)?'
    return re.findall(regex,s)

bufsizes = [20, 4096, 1 << 20]
cnt = [10, 100, 1000]
results = []

for s in bufsizes:
    for c in cnt:
        args = ['../../Build/linux-x86_64.debug/tests/serializer_gtest',
            '--buf-size={}'.format(s),
            '--cnt={}'.format(c),
            '--gtest_filter=SerializationTest.stringTbl']
        print args
        output = subprocess.check_output(args)
        print output
        # parse out memcpy_time, th_time, and written
        output = output.split('\n')
        for line in output:
            if re.search( r'memcpy serilization time', line):
                memcpy_time = long(extract_number(line)[0])
            elif re.search( r'Thrift serilization time', line):
                th_time = long(extract_number(line)[0])
                written = long(extract_number(line)[1])
        # append to results
        result = [s, c, memcpy_time, th_time, written]
        results.append(result)
        print result

print tabulate(results, ['bufsize', 'count', 'memcpy time', 'thrift time', 'written'])
