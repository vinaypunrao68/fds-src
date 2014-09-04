#!/bin/bash
# BUILD_PATH=/var/lib/jenkins/build
BUILD_PATH=/home/jenkins/build
cd $BUILD_PATH
# Sample date output
BUILD_FOLDER_DATE="fds-dev-`date +%F-%H-%M`"
BUILD_FOLDER="fds-dev-daily-build-test"
CONFIG="cfg/jenkins.cfg"


# export LD_LIBRARY_PATH=/usr/local/lib:$BUILD_PATH/$BUILD_FOLDER/leveldb-1.12.0:$BUILD_PATH/$BUILD_FOLDER/source/Build/linux-*/lib:$BUILD_PATH/$BUILD_FOLDER/libconfig-1.4.9/lib/.libs/
source ~/.bashrc

export JAVA_HOME=/usr/lib/jvm/java-8-oracle
source /var/lib/jenkins/.env
export LD_LIBRARY_PATH=/usr/local/lib:/home/jenkins/build/fds-dev-daily-build-test/leveldb-1.12.0:/home/jenkins/build/fds-dev-daily-build-test/source/Build/linux-*/lib:/home/jenkins/build/fds-dev-daily-build-test/libconfig-1.4.9/lib/.libs/
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$JAVA_HOME/jre/lib/amd64:$JAVA_HOME/jre/lib/amd64/server
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/apr/lib
env

function restart_fds()
{
    before_dir=`pwd`
    cd $BUILD_FOLDER/source/test/
    ./fds-tool.py -f $CONFIG -d -c
    ./fds-tool.py -f $CONFIG -u
    cd $before_dir
}

function stop_fds()
{
    cd $BUILD_FOLDER/source/test/
    ./fds-tool.py -f $CONFIG -d -c
}

function exit_if_error()
{
    res=$1
    msg=$2
    if [ $res -ne 0 ]; then
        echo "FDS-Build: $res - $msg"

        echo cd ${BUILD_PATH}/${BUILD_FOLDER}/source
        cd ${BUILD_PATH}/${BUILD_FOLDER}/source

        echo tar czvf ${BUILD_FOLDER_DATE}.tar.gz Build/linux-x86_64.debug/bin/
        tar czvf "${BUILD_FOLDER_DATE}.tar.gz" Build/linux-x86_64.debug/bin/ > /dev/null

        echo tar --append zvf "${BUILD_FOLDER_DATE}.tar.gz" /fds/node1/var/logs/ > /dev/null
        tar --append zvf "${BUILD_FOLDER_DATE}.tar.gz" /fds/node1/var/logs/ > /dev/null

        echo tar --append zvf "${BUILD_FOLDER_DATE}.tar.gz" /corefiles/ > /dev/null
        tar --append zvf "${BUILD_FOLDER_DATE}.tar.gz" /corefiles/ > /dev/null


        echo cp ${BUILD_FOLDER_DATE}.tar.gz /opt/fds/cores/smoke_test/
        #cp ${BUILD_FOLDER_DATE}.tar.gz /opt/fds/cores/smoke_test/

        echo "FDS-Build: bin/log/core directory is saved in " \
             "/opt/fds/cores/smoke_test/${BUILD_FOLDER_DATE}.tar.gz"
        echo "FDS-Build: Daily Build completed with errors"
        echo "FDS-Build: Test FAILED!"

        cd $BUILD_FOLDER/source/tools
        fds stop
        exit $res
    fi
}

echo "FDS-Build: Update build root: $BUILD_FOLDER"

# Run git pull
cd $BUILD_FOLDER
echo "FDS-Build: Pulling from github repo"
git pull

# Doing make
echo "FDS-Build: Running Make"
cd libconfig-1.4.9
./configure
cd ../
make
exit_if_error $? "Make failed"

# Running Smoke Test
echo "FDS-Build: Running Smoke Test"
cd source
#test/fds-primitive-smoke.py --smoke_test true --log_level 0 --std_output yes 
test/fds-primitive-smoke.py --smoke_test true --config jenkins.cfg --am_ip 10.1.10.120
exit_if_error $? "Smoke Test failed"

# Running datagen tester
echo "FDS-Build: Running datagen test"
echo "  Cleaning FDS"
#tools/fds cleanstart > /dev/null
restart_fds()
cd test
fail_count=0
for i in `seq 1 3`
do
    ./fds-datagen.py -f cfg/datagen_io.cfg
    if [ $? -gt 0 ]
    then
        ((fail_count += 1))
    fi
done
if [ $fail_count -ge 3 ]
then
    exit_if_error 1 "Datagen test failed"
fi

# Running multi dm tester
echo "FDS-Build: Running Multi DM Test"
echo "  Cleaning FDS"
#../tools/fds cleanstart > /dev/null
restart_fds()
# Loop 3 times
for i in `seq 1 3`;
do
    ./fds-multi-dm-tester.py
    exit_if_error $? "Failed Multi DM Test (Loop $i)"
done

# Building doxygen
cd ..
#echo "FDS-Build: Building doxygen and copy to build.formationds.com:81"
#doxygen tools/fds_doxy.cfg &> /tmp/doxygen.out
#exit_if_error $? "Failed to make doxygen"
#rm -rf /var/www/doxygen/*
#cp -rf /tmp/html/* /var/www/doxygen/

# Build OK!
#tools/fds stop
stop_fds()
echo "FDS-Build: Daily Build completed successfully"
exit 0
