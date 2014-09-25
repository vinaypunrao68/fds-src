#!/bin/bash

echo "Cleaning up the old tree"
rm -rf /tmp/fdsinstall  /tmp/fdsinstall.tar.gz

echo "Making packages"
cd pkg
make
cd ..
make

dest_name=fdsinstall-`date +%Y%m%d-%H%M`.tar.gz

echo "Packaging install files into fdsinstall.tar.gz"
cd /tmp
tar czvf ${dest_name} fdsinstall

echo "Complete, filename:  ${dest_name}"
