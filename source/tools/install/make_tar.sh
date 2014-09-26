#!/bin/bash


env

set



echo "Cleaning up previous packaging tree"
rm -rf /tmp/fdsinstall  /tmp/fdsinstall-*.tar.gz

echo "Making packages"
cd pkg
make

cd ..
make

dest_name=fdsinstall-`date +%Y%m%d-%H%M`.tar.gz

echo "Packaging install files into ${fdsinstall}"
cd /tmp
tar czvf ${dest_name} fdsinstall

echo "Complete, filename:  ${dest_name}"
