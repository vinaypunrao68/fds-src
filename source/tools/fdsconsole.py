#!/usr/bin/env python
import sys

sys.path.append('../test/fdslib/pyfdsp')
sys.path.append('../test/fdslib')

import fdsconsole.console

if __name__ == '__main__':
    cli = fdsconsole.console.FDSConsole()
    cli.init()
    cli.run(sys.argv[1:])
