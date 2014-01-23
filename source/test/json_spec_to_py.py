#!/usr/bin/env python
import sys
import json
import pdb
from pprint import pprint

if len(sys.argv) != 2:
    sys.exit("Usage: " + sys.argv[0] + " <json.spec>")

json_file = sys.argv[1]
print json_file

json_data = open(json_file)
data = json.load(json_data)
pprint(data)
json_data.close()
