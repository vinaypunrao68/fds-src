#!/usr/bin/env python
from gen_json_spec import JSonTestCmdLine
from gen_json_spec import JSonVal
from gen_json_spec import JSonValRandom
from gen_json_spec import JSonTestCfg
from gen_json_spec import JSonTestCfg
from gen_json_spec import JSonTestClient


# create command line object to parse command line parameters and cfg file.
cmd_line = JSonTestCmdLine()

# define test spec using JSonVal and JSonValRandom
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

# create client test config
test_cfg = JSonTestCfg(cmd_line, js_spec)

# create test client instance
test_client = JSonTestClient(test_cfg)

# run test client -> go
test_client.run()
