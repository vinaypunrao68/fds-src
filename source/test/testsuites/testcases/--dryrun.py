# This module allows us to put a "--dryrun" option on
# PyUnit's command line without PyUnit seeing it.
# We'll pick it up later when we parse the command line
# for the qaautotest module.
# See qaautotest.harness.core.get_options()
# See testcases.-q for more explanation.


import sys

# Where is '--dryrun'? We know it's there or we wouldn't be here.
idx = sys._getframe(2).f_locals['names'].index('--dryrun')

# Remove '--dryrun'.
sys._getframe(2).f_locals['names'].pop(sys._getframe(2).f_locals['names'].__len__()-1)

