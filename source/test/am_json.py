#!/usr/bin/env python
#
# Copyright 2014 Formation Data Systems, Inc.

import optparse

def printHeader():
    print "{\n    \"Run-Input\": {\n        \"am-workload\": {\n            \"am-ops\": ["

def printFooter():
    print "            ]\n        }\n    }\n}"

def printStartBody(numOps):
    body = ""
    for i in range(0, numOps):
        body += "                {\n"
        body +=  "                    \"blob-op\": \"startBlobTx\",\n" +\
                "                    \"volume-name\": \"testVol\",\n" +\
                "                    \"blob-name\": \"blob-" + str(i) + "\"\n"
        if i == (numOps - 1):
            body += "                }"
            #body += "                },"
        else:
            body += "                },\n"
    print body

def printUpdateBody(numOps):
    body = ""
    for i in range(0, numOps):
        body += "                {\n"
        body +=  "                    \"blob-op\": \"updateBlob\",\n" +\
                "                    \"volume-name\": \"testVol\",\n" +\
                "                    \"blob-name\": \"blob-" + str(i) + "\",\n" +\
                "                    \"blob-offset\": 0,\n" +\
                "                    \"data-length\": 4096\n"
        if i == (numOps - 1):
            body += "                }"
        else:
            body += "                },\n"
    print body

def printGetBody(numOps):
    body = ""
    for i in range(0, numOps):
        body += "                {\n"
        body +=  "                    \"blob-op\": \"getBlob\",\n" +\
                "                    \"volume-name\": \"testVol\",\n" +\
                "                    \"blob-name\": \"blob-" + str(i) + "\",\n" +\
                "                    \"blob-offset\": 0,\n" +\
                "                    \"data-length\": 4096\n"
        if i == (numOps - 1):
            body += "                }"
        else:
            body += "                },\n"
    print body

def printJson():
    printHeader()
    #printStartBody(1)
    #printUpdateBody(5000)
    printGetBody(5000)
    printFooter()

if __name__ == '__main__':
    #
    # Parse command line options
    #
    parser = optparse.OptionParser("usage: %prog [options]")
    parser.add_option("-n", "--num", dest="num",
                      help="somenum")
    (options, args) = parser.parse_args()
    numOps = options.num

    printJson()
