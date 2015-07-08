#~/bin/bash

CODE_WORD="resetme"

if [[ "$1". == "${CODE_WORD}". ]] 
then

    dd if=/dev/zero of=/dev/sda1 status=none

    for disk in /dev/sd[b-z]
    do
       echo "Processing ${disk}"
       dd if=/dev/zero of=${disk} bs=1G count=1 status=none
    done
else
    echo "usage:  ./${0##*/} <code_word>    Look at the script to find the code word"
fi
