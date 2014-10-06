#!/usr/bin/env python
import sys
import os
dirname = os.path.dirname(os.path.abspath(os.curdir))
sys.path.insert(0,'{}/test/fdslib/pyfdsp/'.format(dirname))
sys.path.insert(0,'{}/test/fdslib/'.format(dirname))
sys.path.insert(0,'{}/test/'.format(dirname))

import fdsconsole.console

if __name__ == '__main__':
    cli = fdsconsole.console.FDSConsole()
    cli.init()
    cli.run(sys.argv[1:])
