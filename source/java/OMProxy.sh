#!/bin/bash

export CLASSPATH=.
for i in `ls /fds/lib/java/fds-1.0-bin/fds-1.0/*.jar`
do
    export CLASSPATH=${CLASSPATH}:${i}
done

java -cp ${CLASSPATH} com.formationds.spike.om.OMProxy
