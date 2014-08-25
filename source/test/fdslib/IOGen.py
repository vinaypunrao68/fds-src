#!/usr/bin/env python
import subprocess
from optparse import OptionParser
from os import listdir
from os.path import isfile, join
import time
import process
import logging

log = logging.getLogger(__name__)

def send_cmd(cmd):
    output = subprocess.check_output(cmd)
    log.info("send_cmd: {}".format(cmd))

class PutGenerator:
    def __init__(self,
                 endpoint,
                 bucket,
                 dir,
                 repeat_cnt=1):
        self.endpoint = endpoint
        self.bucket = bucket
        self.dir = dir
        self.repeat_cnt = repeat_cnt

    def run(self):
        log.info("Starting PutGenerator")
        # create a bucket
        cmd = "curl -v -X POST http://{}/{}".format(self.endpoint, self.bucket)
        send_cmd(cmd.split())

        files = [f for f in listdir(self.dir) if isfile(join(self.dir,f))]
        for cnt in xrange(self.repeat_cnt):
            for f in files:
                fpath = join(self.dir,f)
                cmd = "curl -v -X POST --data-binary @{} http://{}/{}/{}".format(fpath, self.endpoint, self.bucket, f)
                send_cmd(cmd.split())
        log.info("PutGenerator completed")


class GetGenerator:
    def __init__(self,
                 endpoint,
                 bucket,
                 dir,
                 repeat_cnt=1,
                 sleep = 0):
        self.endpoint = endpoint
        self.bucket = bucket
        self.dir = dir
        self.repeat_cnt = repeat_cnt
        self.sleep = sleep

    def run(self):
        log.info("Starting GetGenerator")
        files = [f for f in listdir(self.dir) if isfile(join(self.dir,f))]
        for cnt in xrange(self.repeat_cnt):
            for f in files:
                cmd = "curl -v http://{}/{}/{}".format(self.endpoint, self.bucket, f)
                send_cmd(cmd.split())
                if (self.sleep != 0):
                    time.sleep(self.sleep)
        log.info("GetGenerator completed")

if __name__ == "__main__":
    process.setup_logger()
    usage = "usage: %prog [options]"
    parser = OptionParser(usage)
    parser.add_option("-o", dest="op", default = "put", help="[put|get]")
    parser.add_option("-d", dest="dir", default = ".", help="directory")
    parser.add_option("-b", dest="bucket", default = "b1", help="bucket")
    parser.add_option("-r", dest="repeat_cnt", default = "1", help="repeat count")

    (options, args) = parser.parse_args()

    if len(args) < 1:
        parser.error("Incorrect number of args")

    log.info('options: {}'.format(options))
    log.info('endpoint: {}'.format(args[0]))

    if options.op == 'put':
        p = PutGenerator(args[0], options.bucket, options.dir,
                         int(options.repeat_cnt))
    else:
        p = GetGenerator(args[0], options.bucket, options.dir, int(options.repeat_cnt))
    p.run()