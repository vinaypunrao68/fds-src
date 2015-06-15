#!/bin/bash

MY_DIR="$(dirname "${0}")"
FDS_SRC="${MY_DIR}"/../../..
JAVA_LIB="${FDS_SRC}"/Build/linux-x86_64.debug/lib/java
CLASSPATH="${CLASSPATH}":"${JAVA_LIB}"/\*

java -cp ${CLASSPATH} com.formationds.spike.om.OMProxy
