#!/bin/bash
target=fds_api
thrift=../../Build/linux-x86_64.debug/bin/thrift
mkdir -p $target
$thrift --gen py -r -I ../fdsp -out $target ../fdsp/config_api.thrift
$thrift --gen py -r -I ../fdsp -out $target ../fdsp/am_api.thrift
cat > $target/__init__.py << SCRIPT_END
from os.path import dirname
import sys
sys.path.append(dirname(__file__))
SCRIPT_END
