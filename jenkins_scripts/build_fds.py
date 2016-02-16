#!/usr/bin/env python
#
# This script builds any necessary dependencies in the root of fds-src
# It is intended to work around assumptions made by our make infrastructure
# about what all needs to be built in every environment.
#
# This is run from Jenkins at the start of any build job
#
import os
import build_lib
from optparse import OptionParser

# Some reused variables
SOURCE_DIR="source"

parser = OptionParser()
parser.add_option("-r", "--release-build-enable", dest = "release_build_enable", action = "store_true", default = False, help = "Enable build release")
parser.add_option("-c", "--coverage", dest = "coverage", action = "store_true", default = False, help = "Enable code coverage reporting.")
parser.add_option("-q", "--quiet", dest = "quiet", action = "store_true", default = False, help = "Hide compilation command-lines.")
(options, args) = parser.parse_args()

os.environ["JAVA_HOME"] = "/usr/lib/jvm/java-8-oracle"
os.environ["npm_config_loglevel"] = "error"
os.environ["JENKINS_URL"] = "true"

build_lib.create_lockfiles()
cmd = 'make fastb=1 threads=13'
if options.release_build_enable:
    cmd += ' BUILD=release'
else:
    cmd += ' CCACHE=1'
if options.coverage:
    cmd += ' COVERAGE=1'
if not options.quiet:
    cmd += ' VERBOSE=1'

can_build = False

#If the 'source' directory does not exist, make an attempt to locate it.
if not os.path.isdir ("./" + SOURCE_DIR):
    base = os.path.dirname (os.path.abspath(__file__))

    while len(base) > 1:
        new_base = os.path.dirname (base)
        if os.path.isdir (new_base + "/" + SOURCE_DIR):
            os.chdir (new_base)
            can_build = True
            break
        base = new_base

    if not can_build:
        print "Unable to find a usable fds-src tree.  Can not continue."
        exit (2)

exit (build_lib.shell_retry(cmd))
