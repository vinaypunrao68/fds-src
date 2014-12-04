#!/bin/bash

curl -X PUT -d@up.json http://localhost:8080/abc
curl -X PUT -d@qual.json http://localhost:8080/abc
curl -X PUT -d@down.json http://localhost:8080/abc
curl -X PUT -d@up.json http://localhost:8080/abc
curl -X PUT -d@qual.json http://localhost:8080/abc
curl -X PUT -d@integ.json http://localhost:8080/abc
curl -X PUT -d@down.json http://localhost:8080/abc
curl -X PUT -d@deploy.json http://localhost:8080/abc
curl -X PUT -d@func.json http://localhost:8080/abc
curl -X PUT -d@down.json http://localhost:8080/abc
curl -X PUT -d@qual.json http://localhost:8080/abc
curl -X PUT -d@func.json http://localhost:8080/abc

