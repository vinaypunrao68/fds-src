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
import time

def create_list_from_list(template_list, count, rand):
    i = 0
    res_list = []
    list_cur = 0
    list_len = len(template_list)
    while i < count:
        if rand == True:
            idx = random.randint(0, list_len - 1)
        else:
            idx = list_cur
            list_cur += 1
            if list_cur == list_len:
                list_cur = 0
        res_list.append(template_list[idx])

        i += 1
    return res_list

def create_list_from_struct(template_struct, count):
    i = 0
    res_list = []
    while i < count:
        res_list.append(template_struct)
        i += 1
    return res_list

def generate_random_string(length=8):
    res = ''
    for i in range(length):
        res = res + random.choice(string.printable)
    return res

def generate_mmh3_4k_random_data(data_size, check_value):
    if check_value == None:
        rand_str_data = generate_random_string(data_size)
    else:
        rand_str_data = check_value
    mmh3_val = mmh3.hash128(rand_str_data)
    mmh3_val = "{0:#0{1}x}".format(mmh3_val, 34)
    mmh3_val.format(34)
    return (mmh3_val, rand_str_data)

def generate_uuid(data_size, check_value):

    while True:
        if check_value == None:
            rand_str_data = 'node_' + generate_random_string(8)
        else:
            rand_str_data = check_value
        mmh3_val      = mmh3.hash(rand_str_data)
        mmh3_val      = mmh3_val & 0xFFFFFFFF
        mmh3_val = "{0:#0{1}x}".format(mmh3_val, 10)
        mmh3_val.format(10)

        if check_value == None:
            if mmh3_val not in generate_uuid_dict:
                generate_uuid_dict[mmh3_val] = rand_str_data
                break
        else:
            break

    return (mmh3_val, rand_str_data)
generate_uuid_dict = { }

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
            (key_hex_str, val) = self.generator_function(self.generator_arg, None)
#(key, val) = self.generator_function(self.generator_arg, None)
#key_hex_str = "{0:#0{1}x}".format(key, 34)

            self.cur_key_hex_str = key_hex_str
            self.cur_value = val

            return key_hex_str
        else:
            self.count = 0
            (tkey, tval) = self.generator_function(self.generator_arg, self.cur_value)
#tkey = "{0:#0{1}x}".format(tkey, 34)
#tkey.format(34)
            assert tkey == self.cur_key_hex_str
            return self.cur_value
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

        if self.count == 0:
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
        saved_keys = self.keyval_stored.get_saved_list()
        assert len(saved_keys) > 0
        res = saved_keys.pop()
        return res

class JSonKeyValTri(JSonKeyVal):
    def __init__(self, generator, generator_arg, values_count, tri_vals):
        JSonKeyVal.__init__(self, generator, generator_arg, values_count)
        self.tri_values = tri_vals
        self.cur_tri_val = None

    def to_json(self):
        if self.count == 0:
            self.count = 1
            (key, val) = self.generator_function(self.generator_arg, None)
            key_hex_str = "{0:#0{1}x}".format(key, 34)
            tri_val = self.tri_values.to_json()

            self.cur_key_hex_str = key_hex_str
            self.cur_value = val
            self.cur_tri_val = tri_val

            return self.cur_tri_val
        elif self.count == 1:
            self.count = 2
            return self.cur_key_hex_str
        else:
            assert self.count == 2
            self.count = 0
            (tkey, tval) = self.generator_function(self.generator_arg, self.cur_value)
            tkey = "{0:#0{1}x}".format(tkey, 34)
            tkey.format(34)
            assert tkey == self.cur_key_hex_str
            return self.cur_value

    def get_key(self):
        if self.count == 0:
            return self.cur_key_hex_str
        return None

    def get_value(self):
        if self.count == 0:
            return self.cur_value
        return None

    def get_tri(self):
        return self.cur_tri_val


class JSonKeyValTriStored(JSonKeyValTri):
    def __init__(self, generator, generator_arg, values_count, tri_vals):
        JSonKeyValTri.__init__(self, generator, generator_arg, values_count, tri_vals)
        self.saved_key_list = []
        self.saved_key_dict = {}
        self.saved_tri_list = []

    def to_json(self):
        res = super(JSonKeyValTriStored, self).to_json()
        key = self.get_key()
        val = self.get_value()
        tri = self.get_tri()

        if self.count == 0:
            self.saved_key_list.append(key)
            self.saved_key_dict[key] = val
            self.saved_tri_list.append(tri)
        return res

    def get_saved_list(self):
        return (self.saved_key_list, self.saved_tri_list)

class JSonKeyValTriRetrive(object):
    def __init__(self, json_keyval_stored):
        self.keyval_stored = json_keyval_stored
        self.keyval_cur    = 0
        self.count         = 0

    def to_json(self):
        (saved_keys, saved_tri) = self.keyval_stored.get_saved_list()
        if self.count == 0:
            assert len(saved_tri) > 0
            self.count = 1
            res = saved_tri.pop()
        else:
            assert len(saved_keys) > 0
            self.count = 0
            res = saved_keys.pop()
        return res
#XXX: provide a retrive class to retrive key-pair in randommize order, assuming
#     all key-pair-stored are generated.

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
        self.parser = argparse.ArgumentParser()
        self.parser.add_argument("--client_id",
                            help="unique client test ID, def 0")
        self.parser.add_argument("--config",
                            help="test case configuration file.")
        self.parser.add_argument("--spec_cnt", \
                            help="generate n number of unique json spec, def all.")
        self.parser.add_argument("--sort_keys", \
                            help="sort json keys in json spec [0|1], def 0.")
        self.parser.add_argument("--print_pretty",
                            help="pretty print json spec [0|1], def 0.")
        self.parser.add_argument("--dryrun",
                            help="do not make http/curl call to server [0|1], def 0")
        self.parser.add_argument("--verbose",
                            help="print json spec and curl commands, def 0")
        self.parser.add_argument("--seed",
                            help="seed number for random generator, def time()")
        self.parser.add_argument("--node_per_call",
                            help="Number of node per call to adapter, def 1")
    def parse(self):
        args = self.parser.parse_args()

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

        if args.verbose == "1":
            self.cmd_verbose = True
        else:
            self.cmd_verbose = False

        if args.seed:
            self.cmd_seed = int(args.seed)
        else:
            self.cmd_seed = time.time()

        if args.node_per_call:
            self.node_per_call = int(args.node_per_call)
        else:
            self.node_per_call = 1
        self.parser_args = args

    def get_args(self):
        return self.parser_args

    def get_client_id(self):
        return self.cmd_client_id

    def get_spec_cnt(self):
        return self.cmd_spec_cnt

    def get_sort_keys(self):
        return self.cmd_sort_keys

    def get_print_pretty(self):
        return self.cmd_print_pretty

    def get_verbose(self):
        return self.cmd_verbose

    def get_dryrun(self):
        return self.cmd_dryrun

    def get_seed(self):
        return self.cmd_seed

class JSonTestCfg(object):
    def __init__(self, cmd_line, test_spec):
        self.cfg_cmd_line  = cmd_line
        self.cfg_test_spec = test_spec
        tmp_seed = self.cfg_cmd_line.get_seed()
        if tmp_seed != None:
            random.seed(tmp_seed)

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

    def get_verbose(self):
        return self.cfg_cmd_line.get_verbose()

    def get_seed(self):
        return self.cfg_cmd_line.get_seed()

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
        self.js_test_verbose      = test_cfg.get_verbose()
        self.js_test_seed         = test_cfg.get_seed()
        self.js_test_spec         = test_cfg.get_test_spec()
        self.js_result_list       = []
        self.js_result_dict       = {}

    def print_seed(self):
        if self.js_test_seed == None:
            print "Random seed number: None"
        else:
            print "Random seed number: %d" % self.js_test_seed

    def run(self):
        print "Generating %d JSon unique spec." % self.js_test_spec_cnt
        self.print_seed()
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
                if self.js_test_verbose == True:
                    print js_res

                result_dict[js_res] = js_res
                result_list.append(js_res)
                i += 1
        print "\nTotal unique test spec generated: %d" % self.js_test_spec_cnt
        self.print_seed()
        self.js_result_list = result_list
        self.js_result_dict = result_dict

        if self.js_test_verbose:
            curl_verbose = "-v"
        else:
            curl_verbose = ""

        if self.js_test_dryrun == False:
            for json_cmd in result_list:
                call(["curl", curl_verbose, "-X", "POST", "-d", \
                      json_cmd, "http://localhost:8000/abc"])
                #call(["curl", "-v", "-X", "PUT", "-d",  \
                #     json_cmd, "http://localhost:8000/abc"])
                #call(["curl", "-v", "-X", "POST", "-d", \
                #     json_cmd, "http://localhost:8000/abc/def"])
                #call(["curl", "-v", "-X", "PUT", "-d",  \
                #     json_cmd, "http://localhost:8000/abc/def"])
        print "\nTotal unique test spec called to curl: %d" % self.js_test_spec_cnt
        self.print_seed()


###############################################################################
# The test for the test.

cmd_line = JSonTestCmdLine()
cmd_line.parse()

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
list_thpool_math    = create_list_from_list([math_cmd_list1, math_cmd_list2], 4, True)

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
