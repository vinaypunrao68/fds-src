#!/usr/bin/env python
#
# Copyright 2014 Formation Data Systems, Inc.

import os
import sha
import optparse
from random import randint

def printHeader():
    if options.commit_log:
        print "{\n    \"Run-Input\": {\n        \"commit-log-workload\": {\n            \"commit-log-ops\": ["
    else:
        print "{\n    \"Run-Input\": {\n        \"tvc-workload\": {\n            \"tvc-ops\": ["

def printFooter():
    print "            ]\n        }\n    }\n}"

def printStartTxBody():
    body = ""
    for i in range(startId, startId + numOps):
        body += "                {\n" +\
                "                    \"blob-op\": \"startTx\",\n" +\
                "                    \"tx-id\": " + str(i) + ",\n" +\
                "                    \"blob-name\": \"blob-" + str(i) + "\"\n" +\
                "                },\n"
    print body

def printUpdateMetaTxBody():
    body = ""
    for i in range(startId, startId + numOps):
        body += "                {\n" +\
                "                    \"blob-op\": \"updateMetaTx\",\n" +\
                "                    \"tx-id\": " + str(i) + ",\n" +\
                "                    \"meta-data\": {\n" +\
                "                         \"descr\": \"Favorite movie blob-" + str(i) + "\",\n" +\
                "                         \"tags\": \"Horror, Alien\"\n" +\
                "                     }\n" +\
                "                },\n"
    print body

def printUpdateTxBody():
    body = ""
    for i in range(startId, startId + numOps):
        numUpdates = randint(1, 5);
        for j in range(0, numUpdates):
            objId = "0x" + sha.new(os.tmpnam()).hexdigest()
            offset = randint(0, 1000) * 4096 # randint(0, 1 * 1024 * 1024)
            length = 4096 #randint(512, 1 * 1024 * 1024)
            body += "                {\n" +\
                    "                    \"blob-op\": \"updateTx\",\n" +\
                    "                    \"tx-id\": " + str(i) + ",\n" +\
                    "                    \"obj-list\": {\n" +\
                    "                         \"offset\": " + str(offset) + ",\n" +\
                    "                         \"obj-id\": \"" + objId + "\",\n" +\
                    "                         \"length\": " + str(length) + "\n" +\
                    "                     }\n" +\
                    "                },\n"
    print body

def printCloseBody():
    body = ""
    percentAbort = 15   # percent chance that request will be aborted
    for i in range(startId, startId + numOps):
        opStr = "abortTx" if (randint(0, 99) < percentAbort) else "commitTx"
        body += "                {\n" +\
                "                    \"blob-op\": \"" + opStr + "\",\n" +\
                "                    \"blob-name\": \"blob-" + str(i) + "\",\n" +\
                "                    \"tx-id\": " + str(i) + "\n" +\
                "                }"

        if options.commit_log and opStr == "commitTx":
            body += ",\n" +\
                    "                {\n" +\
                    "                    \"blob-op\": \"purgeTx\",\n" +\
                    "                    \"tx-id\": " + str(i) + "\n" +\
                    "                }"

        if i != (startId + numOps - 1):
            body += ",\n"

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
    printStartTxBody()
    printUpdateTxBody()
    printUpdateMetaTxBody()
    printCloseBody()

    #printGetBody(5000)
    printFooter()

if __name__ == '__main__':
    #
    # Parse command line options
    #
    parser = optparse.OptionParser("usage: %prog [options]")
    parser.add_option("-s", "--start", dest="start", type="int", help="starting identifier")
    parser.add_option("-n", "--num", dest="num", type='int', help="somenum")
    parser.add_option("-c", "--commit-log", action="store_true",
            help="generate json for commit log test")
    (options, args) = parser.parse_args()

    startId = options.start if options.start else 0
    numOps = options.num if options.num else 10

    printJson()
