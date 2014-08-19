#!/bin/bash
cd /home/jenkins/build/fds-dev-daily-build-test/
source/tools/fds cleanstart
cd s3-tests
S3TEST_CONF=myconf.conf ./virtualenv/bin/nosetests -d -v -a '!fails_on_rgw' 2> test_output.txt
tail test_output.txt
cd ..
source/tools/fds stop
