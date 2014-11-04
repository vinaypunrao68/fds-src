#!/bin/sh

export JAVA_HOME=/usr/lib/jvm/java-8-oracle
export BUILD_DIR=../Build/linux-x86_64.debug

export DEMO_APP=com.formationds.demo.Main

export CLASSPATH=.
export CLASSPATH=${CLASSPATH}:../lib/java/classes
for i in `ls ${BUILD_DIR}/lib/java/fds-1.0-bin/fds-1.0/*.jar`
do
	export CLASSPATH=${CLASSPATH}:$i
done

${JAVA_HOME}/bin/java -cp ${CLASSPATH} ${DEMO_APP} $*
