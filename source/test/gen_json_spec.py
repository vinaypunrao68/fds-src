#!/usr/bin/env python
"""@package docstring
Documentation for gen_json_spec

Generate JSon test spec with the following rules:
1) Serially - each element in JSonVal is generated serially.
2) Combinatorial - each element of JSonVal is generated combinatorially.
3) Incrementally - each element of JSonTestID is generated incrementally
                   from initial value.
"""
import sys
import json
import types
import random
import pdb
import argparse
from subprocess import call
from pprint import pprint
import make_json_serializable
import ConfigParser
import mmh3
import os
import string

def create_list_of_lists(template_lists, count, rand):
    i = 0
    res_lists = []
    list_cur = 0
    list_len = len(template_lists)
    while i < count:
        if rand == True:
            idx = random.randint(0, list_len - 1)
        else:
            idx = list_cur
            list_cur += 1
            if list_cur == list_len:
                list_cur = 0
        res_lists.append(template_lists[idx])

        i += 1
    return res_lists


def generate_random_string(length=8):
    res = ''
    for i in range(length):
        res = res + random.choice(string.printable)
    return res

def generate_mmh3_4k_random_data(number_of_chunk, check_value):
    if check_value == None:
        rand_str_data = generate_random_string(number_of_chunk *
                                               generate_mmh3_4k_random_data_chunk_size)
    else:
        rand_str_data = check_value
    mmh3_val      = mmh3.hash128(rand_str_data)
#   print "========================================================="
#   foo = hex(mmh3_val)
#   foo = foo[:-1]
#   print foo
#   print rand_str_data
#   print "========================================================="
    return (mmh3_val, rand_str_data)

# making it smaller for testing
# generate_mmh3_4k_random_data_chunk_size = 4096
generate_mmh3_4k_random_data_chunk_size = 8

class JSonVal(object):
    def __init__(self, values):
        self.values = values
        self.counter = 0

    def to_json(self):
        idx = self.counter
        self.counter += 1
        if self.counter == len(self.values):
            self.counter = 0
        return self.values[idx]

    def get_values(self):
        return self.values

class JSonValRandom(JSonVal):
    def to_json(self):
        idx = random.randint(0, len(self.values) - 1)
        return self.values[idx]

class JSonHexRandom(JSonVal):
    def to_json(self):
        return '0x%030x' % random.randrange(16**32)

class JSonKeyVal(JSonVal):
    def __init__(self, generator, generator_arg, values_count):
        self.generator_function = generator
        self.generator_arg      = generator_arg
        self.generator_count    = values_count
        self.count              = 0
        self.cur_key_hex_str    = None
        self.cur_value          = None
    def to_json(self):
        if self.count == 0:
            self.count = 1
            (key, val) = self.generator_function(self.generator_arg, None)
            key_hex_str = hex(key)
            key_hex_str = key_hex_str[:-1]

            self.cur_key_hex_str = key_hex_str
            self.cur_value = val

            return key_hex_str
        else:
            self.count = 0
            (tkey, tval) = self.generator_function(self.generator_arg, self.cur_value)
            tkey = hex(tkey)
            tkey = tkey[:-1]
            assert tkey == self.cur_key_hex_str
            return self.cur_value
        #return [key_hex_str, val]
    def get_values(self):
        return [None] * self.generator_count

    def get_key(self):
        return self.cur_key_hex_str
    def get_value(self):
        if self.count == 0:
            return self.cur_value
        return None


class JSonKeyValStored(JSonKeyVal):
    def __init__(self, generator, generator_arg, values_count):
        JSonKeyVal.__init__(self, generator, generator_arg, values_count)
        self.saved_key_list = []
        self.saved_key_dict = {}

    def to_json(self):
        res = super(JSonKeyValStored, self).to_json()
        key = self.get_key()
        val = self.get_value()

        if key != None and val != None:
            self.saved_key_list.append(key)
            self.saved_key_dict[key] = val

        return res
    def get_saved_list(self):
        return self.saved_key_list

class JSonKeyValRetrive(object):
    def __init__(self, json_keyval_stored):
        self.keyval_stored = json_keyval_stored
        self.keyval_cur    = 0

    def to_json(self):
        saved_list = self.keyval_stored.get_saved_list()
        assert self.keyval_cur < len(saved_list)
        idx = self.keyval_cur;
        self.keyval_cur += 1
        return saved_list[idx]

class JSonTestID(JSonVal):
    def to_json(self):
        next_idx = self.counter
        self.counter += 1
        if self.counter == (1 << 24):
            sys.exit("ERROR: test case ID reached maximum")
        return (self.values[0] << 24) + next_idx

class JSonTestCmdLine(object):
    def __read_config_file(self, config_file):
        # XXX: Add curl command to be run, in config file
        # XXX: Add client test id in config file, command line will override
        # XXX: have hostname and path
        # XXX: ctrl/data put/post
        if config_file is not None:
            # config = ConfigParser.ConfigParser()
            # config.read(config_file)
            # print "reading config file %s" % config_file
            print "config file not supported"
            sys.exit(1)

    def __init__(self):
        # parse command line arguments
        parser = argparse.ArgumentParser()
        parser.add_argument("--client_id",
                            help="unique client test ID, def 0")
        parser.add_argument("--config",
                            help="test case configuration file.")
        parser.add_argument("--spec_cnt", \
                            help="generate n number of unique json spec, def all.")
        parser.add_argument("--sort_keys", \
                            help="sort json keys in json spec [0|1], def 0.")
        parser.add_argument("--print_pretty",
                            help="pretty print json spec [0|1], def 0.")
        parser.add_argument("--dryrun",
                            help="do not make http/curl call to server [0|1], def 0")
        args = parser.parse_args()

        if args.client_id:
            self.cmd_client_id = JSonTestID([int(args.client_id)])
        else:
            self.cmd_client_id = JSonTestID([0])

        if args.config:
            self.__read_config_file(args.config)
        else:
            self.__read_config_file(None)

        if args.spec_cnt:
            self.cmd_spec_cnt = int(args.spec_cnt)
        else:
            self.cmd_spec_cnt = 0

        if args.sort_keys == "1":
            self.cmd_sort_keys = True
        else:
            self.cmd_sort_keys = False

        if args.print_pretty == "1":
            self.cmd_print_pretty = 4
        else:
            self.cmd_print_pretty = None

        if args.dryrun == "1":
            self.cmd_dryrun = True
        else:
            self.cmd_dryrun = False

    def get_client_id(self):
        return self.cmd_client_id

    def get_spec_cnt(self):
        return self.cmd_spec_cnt

    def get_sort_keys(self):
        return self.cmd_sort_keys

    def get_print_pretty(self):
        return self.cmd_print_pretty

    def get_dryrun(self):
        return self.cmd_dryrun

class JSonTestCfg(object):
    def __init__(self, cmd_line, test_spec):
        self.cfg_cmd_line  = cmd_line
        self.cfg_test_spec = test_spec

    def __get_combination(self, ts, comb):
        if type(ts) is dict:
            for key in ts:
                val = ts[key]
                if isinstance(val, JSonVal):
                    values_list = val.get_values()
                    comb       *= len(values_list)
                elif type(val) == dict:
                    comb = self.__get_combination(val, comb)
                elif type(val) == list:
                    comb = self.__get_combination(val, comb)
        elif type(ts) is list:
            for elm in ts:
                if isinstance(elm, JSonVal):
                    values_list = elm.get_values()
                    comb       *= len(values_list)
                elif type(elm) == dict:
                    comb = self.__get_combination(elm, comb)
                elif type(elm) == list:
                    comb = self.__get_combination(elm, comb)
        return comb

    def get_client_id(self):
        return self.cfg_cmd_line.get_client_id()

    def get_spec_cnt(self):
        cnt  = self.cfg_cmd_line.get_spec_cnt()
        if cnt == 0:
            comb = self.__get_combination(self.cfg_test_spec, 1)
            res = comb
        else:
            res = cnt
        return res

    def get_sort_keys(self):
        return self.cfg_cmd_line.get_sort_keys()

    def get_print_pretty(self):
        return self.cfg_cmd_line.get_print_pretty()

    def get_dryrun(self):
        return self.cfg_cmd_line.get_dryrun()

    def get_test_spec(self):
        return self.cfg_test_spec

class JSonTestClient(object):
    js_test_missed_limit = 100

    def __init__(self, test_cfg):
        self.js_test_id           = test_cfg.get_client_id()
        self.js_test_spec_cnt     = test_cfg.get_spec_cnt()
        self.js_test_sort_keys    = test_cfg.get_sort_keys()
        self.js_test_print_pretty = test_cfg.get_print_pretty()
        self.js_test_dryrun       = test_cfg.get_dryrun()
        self.js_test_spec         = test_cfg.get_test_spec()
        self.js_result_list       = []
        self.js_result_dict       = {}

    def run(self):
        print "Generating %d JSon unique spec." % self.js_test_spec_cnt
        i           = 0
        missed      = 0
        result_list = []
        result_dict = {}
        while i < self.js_test_spec_cnt:

            if self.js_test_print_pretty is None:
                js_res = json.dumps(self.js_test_spec,                      \
                                    sort_keys = self.js_test_sort_keys)
            else:
                js_res = json.dumps(self.js_test_spec,                      \
                                    sort_keys = self.js_test_sort_keys,     \
                                    indent    = self.js_test_print_pretty)

            if js_res in result_dict:
                missed += 1
                if missed >= JSonTestClient.js_test_missed_limit:
                    print ".",
                    missed = 0
            else:
                print js_res
                result_dict[js_res] = js_res
                result_list.append(js_res)
                i += 1
        print "\nTotal unique test spec generated: %d" % self.js_test_spec_cnt
        self.js_result_list = result_list
        self.js_result_dict = result_dict

        if self.js_test_dryrun == False:
            for json_cmd in result_list:
                call(["curl", "-v", "-X", "POST", "-d", \
                      json_cmd, "http://localhost:8000/abc"])
                call(["curl", "-v", "-X", "PUT", "-d",  \
                      json_cmd, "http://localhost:8000/abc"])
                call(["curl", "-v", "-X", "POST", "-d", \
                      json_cmd, "http://localhost:8000/abc/def"])
                call(["curl", "-v", "-X", "PUT", "-d",  \
                      json_cmd, "http://localhost:8000/abc/def"])


###############################################################################
# The test for the test.

cmd_line = JSonTestCmdLine()

syscall_cmd         = JSonVal(['open', 'close', 'read', 'write'])
syscall_path        = JSonVal(['/dev/null', '/tmp/foo', '/tmp/foo2'])
boost_cmd           = JSonVal(['add', 'sub', 'print'])
boost_array_index0  = JSonVal(['0', '1'])
boost_array_index1  = JSonVal(['0', '1'])
boost_value         = JSonVal(['100', '200'])

math_cmd            = JSonVal(['add', 'subtract'])
math_operand_left   = JSonVal(['100', '200'])
math_operand_right  = JSonVal(['10', '20'])

math_cmd_list1      = [math_cmd, math_operand_left, math_operand_right]
math_cmd_list2      = [math_cmd, math_operand_left]

list_thpool_syscall = [[syscall_cmd, syscall_path, 'FOO', 'BAR'], \
                       [syscall_cmd, syscall_path, 'FOO', 'BAR']]
list_thpool_boost   = [boost_cmd, boost_array_index0, boost_array_index1, boost_value]
list_thpool_math    = create_list_of_lists([math_cmd_list1, math_cmd_list2], 4, True)

dict_threadpool     = {'thpool-syscall' : list_thpool_syscall,
                       'thpool-boost'   : list_thpool_boost,
                       'thpool-math'    : list_thpool_math}

dict_server_load    = {'threadpool'     : dict_threadpool}
dict_run_input      = {'Server-Load'    : dict_server_load}
js_spec             = {'Run-Input'      : dict_run_input,
                       'Test-ID'        : cmd_line.get_client_id()} # get unique client ID

# Create client test config.
# Bind the command line argument with the JSon spec with objects.
test_cfg = JSonTestCfg(cmd_line, js_spec)

# Create test client instance.
test_client = JSonTestClient(test_cfg)

# Run test client.
#
# 1) Generate <n> number of unique JSon spec given test configuration.
# 2) Make http calls to server with the generated JSon spec.
# test_client.run()

def gen_json_spec_test_create_list_of_lists():
    list_1 = ["1", "2", "3"]
    list_2 = ["a", "b"]
    create_test_lists = [list_1, list_2]
    res_lists = create_list_of_lists(create_test_lists, 10, True)
    print res_lists

def gen_json_spec_selftest():
    comb =  len(syscall_cmd.get_values()) * \
            len(syscall_path.get_values()) * \
            len(boost_cmd.get_values()) * \
            len(boost_array_index0.get_values()) * \
            len(boost_array_index1.get_values()) * \
            len(boost_value.get_values()) * \
            len(math_cmd.get_values()) * \
            len(math_operand_left.get_values()) * \
            len(math_operand_right.get_values())
    print "Combination calculated: %d" % comb

def gen_json_spec_test_keyval():
    #(hash_val, rand_data) = generate_mmh3_4k_random_data(1, None)
    #print rand_data
    #print hash_val
    #syscall_data = JSonKeyVal(generate_mmh3_4k_random_data, 1, 1)
    #syscall_data = {"key": ['40947508796434783991892619254342943078']}
    #syscall_data = ['40947508796434783991892619254342943078', 'asdfasdfasdf']
    syscall_data = JSonKeyVal(generate_mmh3_4k_random_data, 1, 1)
    dict_syscall = {"key": [syscall_data, syscall_data]}
    js_res = json.dumps(dict_syscall,                      \
                        sort_keys = True,     \
                        indent    = 4)
    print js_res

# Test functions:
# gen_json_spec_selftest()
# gen_json_spec_test_create_list_of_lists()
# gen_json_spec_test_keyval()
