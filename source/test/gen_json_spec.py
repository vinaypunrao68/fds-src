#!/usr/bin/env python
import sys
import json
import types
import random
import pdb
import argparse
from subprocess import call
from pprint import pprint
import make_json_serializable

parser = argparse.ArgumentParser()
parser.add_argument("--spec_cnt", help="generate n number of unique json spec")
args = parser.parse_args()
if args.spec_cnt:
    json_comb_default = int(args.spec_cnt)
else:
    json_comb_default = 0

random.seed(100)

class JSonVal(object):
    def __init__(self, values):
        self.values = values
        self.counter = 0
        self.random = 1

    def to_json(self):
        if self.random == 1:
            idx = random.randint(0, len(self.values) - 1)
        else:
            idx = self.counter
            self.counter += 1
            if self.counter == len(self.values):
                self.counter = 0
        return self.values[idx]

    def get_values(self):
        return self.values
syscall_cmd = JSonVal(['open', 'close', 'read', 'write'])
syscall_path = JSonVal(['/dev/null', '/tmp/foo', '/tmp/foo2'])

#boost_cmd = JSonVal(['add', 'sub', 'print'])
#boost_array_index0 = JSonVal(['0', '1', '2', '3', '4', '5', '6', '7', '8', '9'])
#boost_array_index1 = JSonVal(['0', '1', '2', '3', '4', '5', '6', '7', '8', '9'])
#boost_value = JSonVal(['100', '200'])

boost_cmd = JSonVal(['add', 'sub', 'print'])
boost_array_index0 = JSonVal(['0', '1', '2'])
boost_array_index1 = JSonVal(['0', '1', '2'])
boost_value = JSonVal(['100', '200'])

math_cmd = JSonVal(['square_root', 'multiply', 'divide', 'add', 'subtract'])
math_operand_left = JSonVal(['100', '200', '300', '400'])
math_operand_right = JSonVal(['10', '20', '30', '40'])

if json_comb_default != 0:
    json_combination = json_comb_default
else:
    json_combination = len(syscall_cmd.get_values()) *          \
                       len(syscall_path.get_values()) *         \
                       len(boost_cmd.get_values()) *            \
                       len(boost_array_index0.get_values()) *   \
                       len(boost_array_index1.get_values()) *   \
                       len(boost_value.get_values()) *          \
                       len(math_cmd.get_values()) *             \
                       len(math_operand_left.get_values()) *    \
                       len(math_operand_right.get_values())

list_thpool_syscall = [syscall_cmd, syscall_path, 'FOO', 'BAR']
list_thpool_boost = [boost_cmd, boost_array_index0, boost_array_index1, boost_value]
list_thpool_math = [math_cmd, math_operand_left, math_operand_right] 

dict_threadpool = {'thpool-syscall' : list_thpool_syscall,
                   'thpool-boost'   : list_thpool_boost,
                   'thpool-math'    : list_thpool_math}

dict_server_load = {'threadpool': dict_threadpool} 
dict_run_input = {'Server-Load' : dict_server_load}
js_spec = {'Run-Input' : dict_run_input,
           'Foo'       : 'Bar'}

# function not used, pretty print json spec.
def parse_js_spec(json_spec, level):
    if type(json_spec) == tuple:
        print "  " * level,
        if type(json_spec[1]) == list:
            open_bracket = ": "
            close_bracket = ""
        elif type(json_spec[1]) == str:
            open_bracket = ": "
            close_bracket = ""
        else:
            open_bracket = " {"
            close_bracket = "}"

        if open_bracket == ": ":
            print "\"" + json_spec[0] + "\"", open_bracket,
        else:
            print "\"" + json_spec[0] + "\"", open_bracket
        parse_js_spec(json_spec[1], level + 1)
        if close_bracket != "":
            print "  " * level,
            print close_bracket,
    elif type(json_spec) == dict:
        for key in json_spec:
            json_tuple = (key, json_spec[key])
            parse_js_spec(json_tuple, level + 1)
            print ","
    elif type(json_spec) == list:
        # print "  " * level,
        # print json_spec,
        res = json.dumps(json_spec)
        print res,
    else:
        # print "  " * level,
        print json_spec,

def parse_json(js_spec):
    print "{"
    parse_js_spec(js_spec, 0)
    print "}"

print "Generating JSon unique spec: %d" % json_combination
json_result_list = []
i = 0
missed = 0
while i < json_combination:

    #parse_json(js_spec)

    js_res = json.dumps(js_spec)

    if js_res in json_result_list:
        missed += 1
        if missed >= 100:
            print ".",
            missed = 0
    else:
        json_result_list.append(js_res)
        print js_res
        i += 1
print "\nTOTAL combination generated: %d" % json_combination

for json_cmd in json_result_list:
    call(["curl", "-v", "-X", "POST", "-d", json_cmd, "http://localhost:8000/abc"])
    call(["curl", "-v", "-X", "PUT", "-d", json_cmd, "http://localhost:8000/abc"])
    call(["curl", "-v", "-X", "POST", "-d", json_cmd, "http://localhost:8000/abc/def"])
    call(["curl", "-v", "-X", "PUT", "-d", json_cmd, "http://localhost:8000/abc/def"])
