#!/bin/bash
function usage
{
    echo "$0 [--debug|-d] [--quiet|-q]"
    exit 1
}

debug=0
quiet=0
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
        *)
        usage        # unknown option
        ;;
    esac
    shift
done

arg=""
if [[ $debug -eq 1 ]]; then
    arg="--debug"
fi

# trigger a scsi rescan
echo "Rescanning disks..."
rescan-scsi-bus > /dev/null 2>&1
if [[ $? -ne 0 ]]; then
    read -n 1 -p "WARNING: Failed rescannning scsi bus. Press any key to continue or ^C to exit."
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

/fds/bin/disk_format.py --format $arg
if [ $? -ne 0 ]; then
    exit $?
fi

# now trigger PM disk rescan by creating /fds/etc/adddisk. This is a file PM watches for CREATE|OPEN events on.
semaphore_file=/fds/etc/adddisk
touch $semaphore_file
if [ $? -ne 0 ]; then
    ret=$?
    echo "Failed triggering PM disk rescan"
    exit $ret
fi
echo "PM disk rescan triggered, verify /fds/dev/disk-map"
