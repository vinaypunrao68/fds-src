#!/bin/bash -e

npm install
CI=true node_modules/bower/bin/bower --allow-root install
node_modules/grunt-cli/bin/grunt release

