#!/bin/bash

echo "Cleaning up the old tree"
rm -rf /tmp/fdsinstall  /tmp/fdsinstall.tar.gz

echo "Making packages"
cd pkg
make
cd ..
make

echo "Packaging install files into fdsinstall.tar.gz"
cd /tmp
tar czvf fdsinstall.tar.gz fdsinstall

echo "Complete!"
