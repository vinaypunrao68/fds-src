#!/bin/bash
fds_dir=/home/monchier/regress/fds-src/source
test_id=`echo $RANDOM`
test_dir=/home/monchier/regress/test-$test_id
nbdvolume=/dev/nbd15

function do_pull() {
    pushd $fds_dir
    git stash
    git pull
    git stash pop
    git log | head > $test_dir/githead
    git branch | head > $test_dir/gitbranch
    popd
}

function do_make() {
    pushd $fds_dir
    export CPLUS_INCLUDE_PATH=$CPLUS_INCLUDE_PATH:/usr/lib/jvm/java-8-oracle/include/:/usr/lib/jvm/java-8-oracle/include/linux ; make clean && make
    if [[ $? -ne 0 ]] ; then
        echo "Build failed" | mail -s "performance regression" matteo@formationds.com
        exit 1
    fi
    popd
}

function package() {
    pushd $fds_dir
        pushd install/pkg
        make
        popd

        pushd install
        make
        popd

        pushd /tmp
        tar czvf fdsinstall.tgz fdsinstall
        popd
    popd

}

function install() {
    tarball=$1
    cp $tarball /tmp/
    pushd /tmp/
    tar xzvf $tarball
        pushd /tmp/fdsinstall
        ./fdsinstall.py -i -o 4
        popd
    popd
}

function bringup() {
    pushd $fds_dir
    ./tools/fds cleanstart
    sleep 10
    popd
}

function bringup2() {
    cp fdstool.cfg /fds/sbin/
    pushd /fds/sbin
    ./fds-tool.py -f fdstool.cfg -d -c -u
    sleep 10
    popd
}

function create_volumes() {
    curl -v -X PUT http://localhost:8000/volume0 
    sleep 10
}

function create_nbd_volumes() {
    pushd $fds_dir
    /fds/bin/fdscli --volume-create volume0 -i 1 -s 10240 -p 50 -y blk
    sleep 5
    nbdvolume=`./cinder/nbdadm.py attach localhost volume0`
    echo nbd volume: $nbdvolume
    sleep 5
    /fds/bin/fdscli --volume-modify "volume0" -s 10240 -g 0 -m 0 -r 10
    sleep 10
    popd
}

function run_block_tests() {
    pushd $1
    disks=$nbdvolume # do one disk only
    bsizes="4k 32k"
    jobs="1 4"
    echo $disks
    options="--runtime=30 --ioengine=libaio --iodepth=16 --direct=1 --size=10g "
    writes="--name=writes $options --rw=randwrite"
    reads="--name=reads $options --rw=randread"
    for bs in $bsizes
    do
        for d in $disks
        do
          for j in $jobs
          do
            target=`echo $d | awk -F "/" '{print $3}'`
            fio $writes --filename=$d --bs=$bs --numjobs=$j --output out-block-write-$bs-$j
            fio $reads --filename=$d --bs=$bs --numjobs=$j --output out-block-read-$bs-$j
          done
        done
    done
    popd
}

function run_s3_test() {
    pushd $1
    local nreqs=100000
    #local nreqs=10000
    # for s in 4096 262144 1048576
    for s in 4096
    do
        for i in 1 2 4 6 8 10 12 14 16 18 20 25 30 35 40
        # for i in 20 30
        do
            python3.3 /home/monchier/regress/fds-src/source/test/traffic_gen.py -t $i -n $nreqs -T PUT -s $s -F 1000 -v 1 -u > out-s3-put-$i-$s
            python3.3 /home/monchier/regress/fds-src/source/test/traffic_gen.py -t $i -n $nreqs -T GET -s $s -F 1000 -v 1 -u > out-s3-get-$i-$s
        done
    done
    popd
}

function get_object_size() {
    file=$1
    grep Options $file | awk '{split($0, array, ","); split(array[$6],array2," "); print array2[2]}'
}

###############################################

if [ ! -d $test_dir ]
then
    mkdir $test_dir
fi
echo test directory: $test_dir
echo date > $test_dir/date

branch="master"

# build and make
# do_pull 
# do_make

# install fdsinstall-$branch.tgz

# need to specialize for branch

# s3 tests
bringup2
create_volumes
run_s3_test $test_dir
./postprocess.py $test_dir $test_id s3 $branch

# block tests
bringup2
create_nbd_volumes
run_block_tests $test_dir
./postprocess.py $test_dir $test_id block $branch

