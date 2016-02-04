#!/bin/bash
function usage
{
    echo "$0 [--debug|-d] [--quiet|-q] [--fds-root <fds_root>]"
    exit 1
}

debug=0
quiet=0
fds_root='/fds/'
args=""
while [[ $# > 0 ]]
do
    key="$1"

    case $key in
        -d|--debug)
        debug=1
        ;;
        -q|--quiet)
        quiet=1
        ;;
        -f|--fds-root)
        shift
        fds_root=$1
        args="--fds-root=$fds_root"
        ;;
        *)
        usage        # unknown option
        ;;
    esac
    shift
done

if [ ! -d $fds_root ]; then
    echo "ERROR: directory $fds_root does not exist."
    exit 2
fi

if [[ $fds_root != *\/ ]]; then
    fds_root=${fds_root}/
fi

fds_etc=${fds_root}etc
fds_dev=${fds_root}dev

if [ ! -d $fds_etc -o ! -d $fds_dev ]; then
    echo "ERROR: directory $fds_etc or $fds_dev does not exist."
    exit 2
fi
 
if [[ $debug -eq 1 ]]; then
    args="--debug ${args}"
fi

# trigger a scsi rescan
which rescan-scsi-bus > /dev/null 2>&1
if [[ $? -ne 0 ]]; then
    echo "WARNING: rescan-scsi-bus is not installed. Continuing without rescanning."
else
    echo "Rescanning disks..."
    rescan-scsi-bus > /dev/null 2>&1
    if [[ $? -ne 0 ]]; then
        read -n 1 -p "WARNING: Failed rescannning scsi bus. Press any key to continue or ^C to exit."
    fi
fi

if [[ $quiet -eq 0 ]]; then
    /fds/bin/disk_id.py $arg
    if [ $? -ne 0 ]; then
        exit $?
    fi 
    read -n 1 -p "The output above will be written into /fds/dev/disk-format.conf. Press any key to continue. or ^C to exit." 
fi

/fds/bin/disk_id.py -w $args
if [ $? -ne 0 ]; then
    exit $?
fi

/fds/bin/disk_format.py --format $args
if [ $? -ne 0 ]; then
    exit $?
fi

# now trigger PM disk rescan by creating /fds/etc/adddisk. This is a file PM watches for CREATE|OPEN events on.
semaphore_file=${fds_etc}/adddisk
touch $semaphore_file
if [ $? -ne 0 ]; then
    ret=$?
    echo "Failed triggering PM disk rescan"
    exit $ret
fi
echo "PM disk rescan triggered, verify ${fds_dev}/disk-map"
