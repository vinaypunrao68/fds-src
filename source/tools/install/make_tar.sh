#!/bin/bash

echo "Making packages"
cd pkg
make
cd ..
make

echo "Packaging install files into fdsinstall.tar.gz"
cd /tmp
tar czvf fdsinstall.tar.gz fdsinstall

echo "Complete!"
