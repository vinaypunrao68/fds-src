#!/usr/bin/env python
#
# Copyright 2014 Formation Data Systems, Inc.

import gen_json_spec
from gen_json_spec import JSonTestCmdLine
from gen_json_spec import JSonVal
from gen_json_spec import JSonValRandom
from gen_json_spec import JSonHexRandom
from gen_json_spec import JSonTestCfg
from gen_json_spec import JSonTestCfg
from gen_json_spec import JSonTestClient
from gen_json_spec import JSonKeyValStored
from gen_json_spec import JSonKeyValRetrive

# Create command line object to parse command line parameters and cfg file.
#
# JSonTestCmdLine has the responsibility of parsing the following arguments from cmd line.
#
# $ ./sm_json.py -h
# usage: sm_json.py [-h] [--client_id CLIENT_ID] [--config CONFIG]
#                        [--spec_cnt SPEC_CNT] [--sort_keys SORT_KEYS]
#                        [--print_pretty PRINT_PRETTY] [--dryrun DRYRUN]
#
# optional arguments:
#   -h, --help            show this help message and exit
#   --client_id CLIENT_ID
#                         unique client test ID, def 0
#   --config CONFIG       test case configuration file.
#   --spec_cnt SPEC_CNT   generate n number of unique json spec, def all.
#   --sort_keys SORT_KEYS
#                         sort json keys in json spec [0|1], def 0.
#   --print_pretty PRINT_PRETTY
#                         pretty print json spec [0|1], def 0.
#   --dryrun DRYRUN       do not make http/curl call to server [0|1], def 0
cmd_line = JSonTestCmdLine()

# Define test spec using JSonVal and JSonValRandom
#
# The 'js_spec' dictionary below is the Python representation of a JSon spec with
# JSonVal/JSonValRandom objects that JSonTestClient will use to generate a normal
# JSon spec.
#
# For each JSonVal and JSonValRandom objects, the list passed into the constructor
# will be used to generate the 'value' portion of the JSon spec.
#
# For this particular example, 'js_spec' represents this JSon spec with objects:
#{
#    "Run-Input": {
#        "Server-Load": {
#            "threadpool": {
#                "thpool-syscall": [
#                    syscall_cmd,        <--- ['open', 'close', 'read', 'write']
#                                             1 element of syscall_cmd is insert here
#                                             per JSon spec generated.
#                    syscall_path,       <--- ['/dev/null', '/tmp/foo', '/tmp/foo2']
#                    "FOO",
#                    "BAR"
#                ],
#                "thpool-boost": [
#                    boost_cmd,          <--- ['add', 'sub', 'print']
#                    boost_array_index0, <--- ['0', '1']
#                    boost_array_index1, <--- ['0', '1']
#                    boost_value         <--- ['100', '200']
#                ],
#                "thpool-math": [
#                    math_cmd,           <--- ['add', 'subtract']
#                    math_operand_left,  <--- ['100', '200']
#                    math_operand_right  <--- ['10', '20']
#                ]
#            }
#        }
#    },
#    "Test-ID": JSonTestID               <--- Incremental ID per JSon spec generated.
#}
#
# Example of a JSon spec generated with 'js_spec':
#{
#    "Run-Input": {
#        "Server-Load": {
#            "threadpool": {
#                "thpool-syscall": [
#                    "read",
#                    "/tmp/foo",
#                    "FOO",
#                    "BAR"
#                ],
#                "thpool-boost": [
#                    "sub",
#                    "0",
#                    "0",
#                    "100"
#                ],
#                "thpool-math": [
#                    "add",
#                    "100",
#                    "10"
#                ]
#            }
#        }
#    },
#    "Test-ID": 2290
#}
sm_put_cmd          = JSonVal(['put'])
sm_get_cmd          = JSonVal(['get'])
sm_del_cmd          = JSonVal(['delete'])
sm_vol_id           = JSonVal(['1']) # JSonVal(['1', '2', '3', '4', '5',
                      #         '6', '7', '8', '9', '10'])
sm_obj_data_size    = 8
sm_obj_id_data      = JSonKeyValStored(gen_json_spec.generate_mmh3_4k_random_data,
                                       sm_obj_data_size, 1)
sm_obj_id_data_saved = JSonKeyValRetrive(sm_obj_id_data)
sm_obj_id_data_saved2 = JSonKeyValRetrive(sm_obj_id_data)

sm_cmd_list1        = [sm_put_cmd, sm_vol_id, sm_obj_id_data, \
                       sm_obj_data_size, sm_obj_id_data]
sm_cmd_list2        = [sm_get_cmd, sm_vol_id, sm_obj_data_size, \
                       sm_obj_id_data_saved]
sm_cmd_list3        = [sm_del_cmd, sm_vol_id, sm_obj_data_size, \
                       sm_obj_id_data_saved2]

list_sm_cmd   = gen_json_spec.create_list_of_lists(
    [sm_cmd_list1, sm_cmd_list2],
    9, False)

dict_sm             = {'object-ops'    : list_sm_cmd}

dict_server_load    = {'dp-workload'    : dict_sm}
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
test_client.run()
