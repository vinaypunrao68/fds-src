#!/usr/bin/env python
#
# Copyright 2014 Formation Data Systems, Inc.

import optparse
from random import randint

def printHeader():
    if options.commit_log:
        print "{\n    \"Run-Input\": {\n        \"commit-log-workload\": {\n            \"commit-log-ops\": ["
    else:
        print "{\n    \"Run-Input\": {\n        \"tvc-workload\": {\n            \"tvc-ops\": ["

def printFooter():
    print "            ]\n        }\n    }\n}"

def printStartBody():
    body = ""
    for i in range(0, options.num):
        body += "                {\n"
        body +=  "                    \"blob-op\": \"startTx\",\n" +\
                "                    \"tx-id\": " + str(i) + ",\n" +\
                "                    \"blob-name\": \"blob-" + str(i) + "\"\n"
        if i == (options.num - 1):
            body += "                }"
            #body += "                },"
        else:
            body += "                },\n"
    print body

def printUpdateBody():
    body = ""
    for i in range(0, options.num):
        for j in range(0, randint(1, 1)):
            body += "                {\n"
            body += "                    \"blob-op\": \"updateBlob\",\n" +\
                    "                    \"tx-id\": " + str(i) + ",\n" +\
                    "                    \"volume-name\": \"testVol\",\n" +\
                    "                    \"blob-name\": \"blob-" + str(i) + "\",\n" +\
                    "                    \"blob-offset\": 0,\n" +\
                    "                    \"data-length\": 4096\n"
            if i == (options.num - 1):
                body += "                }"
            else:
                body += "                },\n"
            print body

def printGetBody():
    body = ""
    for i in range(0, options.num):
        body += "                {\n"
        body +=  "                    \"blob-op\": \"getBlob\",\n" +\
                "                    \"volume-name\": \"testVol\",\n" +\
                "                    \"blob-name\": \"blob-" + str(i) + "\",\n" +\
                "                    \"blob-offset\": 0,\n" +\
                "                    \"data-length\": 4096\n"
        if i == (options.num - 1):
            body += "                }"
        else:
            body += "                },\n"
    print body

def printJson():
    printHeader()
    printStartBody()
    printUpdateBody()
    #printGetBody(5000)
    printFooter()

if __name__ == '__main__':
    #
    # Parse command line options
    #
    parser = optparse.OptionParser("usage: %prog [options]")
    parser.add_option("-n", "--num", dest="num", type='int',
                      help="somenum")
    parser.add_option("-c", "--commit-log", action="store_true",
                      help="generate json for commit log test")
    (options, args) = parser.parse_args()

    printJson()
