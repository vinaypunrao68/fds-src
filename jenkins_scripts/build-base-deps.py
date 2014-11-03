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

os.environ["JAVA_HOME"] = "/usr/lib/jvm/java-8-oracle"
os.environ["npm_config_loglevel"] = "error"
os.environ["JENKINS_URL"] = "true"

build_lib.create_lockfiles()
exit (build_lib.shell_retry('make fastb=1 threads=13'))
