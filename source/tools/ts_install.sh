#!/bin/bash
cd /home/nick/fds-src-dev/source

test/fds-bringup.py -v -p
test/fds-bringup.py -i -v -f ~/test_cfgs/local.cfg
