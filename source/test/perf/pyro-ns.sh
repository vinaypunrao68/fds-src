#!/bin/bash
ip=$1
port=$2
#python -Wignore -m Pyro4.naming --host 10.1.10.102 --port 47672
python -Wignore -m Pyro4.naming --host $ip --port $port
