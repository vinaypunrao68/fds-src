# This module allows us to put a "-q" option on
# PyUnit's command line without PyUnit seeing it.
# We'll pick it up later when we discover that
# a qaautotest.ini file has not yet been identified.
# See TestCase.FDSTestCase.setUp()
#
# Before fixing this problem, here's what we saw trying to run the test case
# "TestFDSEnvMgt.TestFDSCreateInstDir" using the PyUnit harness:
# greg@purple:~/fds-src/source/test/testcases$ python -m unittest TestFDSEnvMgt.TestFDSCreateInstDir -q ~/fds-src/source/test/testsuites/BuildSmokeTest.ini
# Traceback (most recent call last):
#   File "/usr/lib/python2.7/runpy.py", line 162, in _run_module_as_main
#     "__main__", fname, loader, pkg_name)
#   File "/usr/lib/python2.7/runpy.py", line 72, in _run_code
#     exec code in run_globals
#   File "/usr/lib/python2.7/unittest/__main__.py", line 12, in <module>
#     main(module=None)
#   File "/usr/lib/python2.7/unittest/main.py", line 94, in __init__
#     self.parseArgs(argv)
#   File "/usr/lib/python2.7/unittest/main.py", line 149, in parseArgs
#     self.createTests()
#   File "/usr/lib/python2.7/unittest/main.py", line 158, in createTests
#     self.module)
#   File "/usr/lib/python2.7/unittest/loader.py", line 130, in loadTestsFromNames
#     suites = [self.loadTestsFromName(name, module) for name in names]
#   File "/usr/lib/python2.7/unittest/loader.py", line 91, in loadTestsFromName
#     module = __import__('.'.join(parts_copy))
# ImportError: No module named -q
#
# Doing an inspection of variable "names" two frames back in the stack we find:
# ['TestFDSEnvMgt.TestFDSCreateInstDir', '-q', '/home/greg/fds-src/source/test/testsuites/BuildSmokeTest.ini']
#
# So it seems that what we need to do is remove both the '-q' option and its argument,
# the qaautotest.ini file, from variable "names" located two frames back in the stack.
# But they remain with sys.argv so we can get them later.


import sys

# Where is '-q'? We know it's there or we wouldn't be here.
idx = sys._getframe(2).f_locals['names'].index('-q')

# Remove '-q' and its argument.
sys._getframe(2).f_locals['names'].pop(sys._getframe(2).f_locals['names'].__len__()-1)
sys._getframe(2).f_locals['names'].pop(sys._getframe(2).f_locals['names'].__len__()-1)

