#!/usr/bin/env python
import sys
import os
import urllib3
import logging
urllib3.disable_warnings()

def start():
    dirname = os.path.dirname(os.path.abspath(os.curdir))
    sys.path.insert(0, os.getcwd())
    sys.path.insert(0,'{}/test/fdslib/pyfdsp/'.format(dirname))
    sys.path.insert(0,'{}/test/fdslib/'.format(dirname))
    sys.path.insert(0,'{}/test/'.format(dirname))
    sys.path.insert(0,'{}/lib/python2.7/dist-packages/fdslib/pyfdsp'.format(dirname))
    sys.path.insert(0,'{}/lib/python2.7/dist-packages/fdslib'.format(dirname))
    sys.path.insert(0,'{}/lib/python2.7/dist-packages'.format(dirname))

    import fdsconsole.console

    logging.captureWarnings(True)
    args=sys.argv[1:]
    fInit = not (len(args) > 0 and args[0] == 'set')
    cli = fdsconsole.console.FDSConsole(fInit, False)
    if fInit:
        cli.init()
    cli.run(args)

if __name__ == '__main__':
    start()
