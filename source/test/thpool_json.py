#!/usr/bin/env python
from gen_json_spec import JSonTestCmdLine
from gen_json_spec import JSonVal
from gen_json_spec import JSonValRandom
from gen_json_spec import JSonTestCfg
from gen_json_spec import JSonTestCfg
from gen_json_spec import JSonTestClient


# Create command line object to parse command line parameters and cfg file.
#
# JSonTestCmdLine has the responsibility of parsing the following arguments from cmd line.
#
# $ ./thpool_json.py -h
# usage: thpool_json.py [-h] [--client_id CLIENT_ID] [--config CONFIG]
#                       [--spec_cnt SPEC_CNT] [--sort_keys SORT_KEYS]
#                       [--print_pretty PRINT_PRETTY] [--dryrun DRYRUN]
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
syscall_cmd         = JSonVal(['open', 'close', 'read', 'write'])
syscall_path        = JSonVal(['/dev/null', '/tmp/foo', '/tmp/foo2'])
boost_cmd           = JSonVal(['add', 'sub', 'print'])
boost_array_index0  = JSonVal(['0', '1'])
boost_array_index1  = JSonVal(['0', '1'])
boost_value         = JSonVal(['100', '200'])

math_cmd            = JSonVal(['add', 'subtract'])
math_operand_left   = JSonVal(['100', '200'])
math_operand_right  = JSonVal(['10', '20'])

list_thpool_syscall = [syscall_cmd, syscall_path, 'FOO', 'BAR']
list_thpool_boost   = [boost_cmd, boost_array_index0, boost_array_index1, boost_value]
list_thpool_math    = [math_cmd, math_operand_left, math_operand_right]

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
test_client.run()

###############################################################################
# The test for the test.
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
# gen_json_spec_selftest()
