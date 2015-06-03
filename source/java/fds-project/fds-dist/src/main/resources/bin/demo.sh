#!/bin/bash

# function to resolve the directory of the script.
# will only work if called *before* any internal 'cd'
# commands.
get_script_dir () {
     SOURCE="${BASH_SOURCE[0]}"
     # While $SOURCE is a symlink, resolve it
     while [ -h "$SOURCE" ]; do
          DIR="$( cd -P "$( dirname "$SOURCE" )" && pwd )"
          SOURCE="$( readlink "$SOURCE" )"
          # If $SOURCE was a relative symlink (so no "/" as prefix, need to resolve it relative to the symlink base directory
          [[ $SOURCE != /* ]] && SOURCE="$DIR/$SOURCE"
     done
     DIR="$( cd -P "$( dirname "$SOURCE" )" && pwd )"
     echo "$DIR"
}


DIR=$(get_script_dir)
echo $DIR $(dirname $DIR)
if [ "source" = $(basename $(dirname $DIR)) ]; then 
    JAVALIB=../Build/linux-x86_64.debug/lib/java
else
    if [ -d ../lib/java ]; then
        JAVALIB=../lib/java
    elif [ -d ../../lib/java ]; then
        JAVALIB=../../lib/java
    else
        echo "Demo script expected to be relative to lib directory (../lib/java)"
        exit 1
    fi
fi
CLASSPATH=${JAVALIB}/\*

java -classpath ${CLASSPATH} \
     -agentlib:jdwp=transport=dt_socket,server=y,suspend=n,address=4006 \
     com.formationds.demo.Main $*
