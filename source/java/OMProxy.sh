#!/bin/bash

export CLASSPATH=.
export CLASSPATH=${CLASSPATH}:/fds/lib/java/fds-1.0-bin/fds-1.0/*
export CLASSPATH=${CLASSPATH}:/fds/lib/java/classes
java -cp ${CLASSPATH} com.formationds.spike.om.OMProxy
