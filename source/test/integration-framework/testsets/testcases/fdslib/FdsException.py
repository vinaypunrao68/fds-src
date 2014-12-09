#!/usr/bin/env python

# Copyright 2014 by Formation Data Systems, Inc.
#


class NodeNotFoundException(Exception):
    pass


class CheckerFailedException(Exception):
    pass


class TimeoutException(Exception):
    pass