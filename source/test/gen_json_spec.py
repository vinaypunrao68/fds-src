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
