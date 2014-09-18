#!/bin/bash
source/tools/fds stop
source/tools/fds cleanstart
cd s3-tests

./s3_test_helper.py
#S3TEST_CONF=myconf.conf ./virtualenv/bin/nosetests -d -v -a '!fails_on_rgw' 2> test_output.txt


#tail test_output.txt
cd ..
#source/tools/fds stop
