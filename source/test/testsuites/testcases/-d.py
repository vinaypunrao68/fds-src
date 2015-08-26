# This module allows us to put a "-d" option on
# PyUnit's command line without PyUnit seeing it.
# We'll pick it up later when we discover that
# a sudo password has not yet been specifid if needed.
# See FdssSetup.FdsLocalEnv.local_connect()
#
# See -q.py for an explanation of the problem being addressed here and its resolution.


import sys

# Where is '-d'? We know it's there or we wouldn't be here.
idx = sys._getframe(2).f_locals['names'].index('-d')

# Remove '-d' and its argument.
sys._getframe(2).f_locals['names'].pop(sys._getframe(2).f_locals['names'].__len__()-1)
sys._getframe(2).f_locals['names'].pop(sys._getframe(2).f_locals['names'].__len__()-1)

