import time
import sys
sys.path.append('../fdslib/')
sys.path.append('../fdslib/pyfdsp/')
from fds_service.ttypes import *
import PlatSvc
import FdspUtils
import process
import random
import string
import os
from optparse import OptionParser
import signal
sys.path.append('../perf')
from RandomFile import RandomFile
import cProfile

def randomword(length):
   return ''.join(random.choice(string.lowercase) for i in range(length))

class Histogram:
    def __init__(self, N = 1, M = 0):
        self.bins = [0,] * (N + 1) # FIXME: refactor name
        self.N = N
        self.M = M
        self.intv = float(M)/N
    def add(self, value):
        if value < self.M:
            index = int(value/self.intv)
            self.bins[index] += 1
        else:
            self.bins[self.N] += 1
    def get(self):
        return self.bins
    # def get_bins(self): # TODO
    #    return [ x * self.intv  for x in range(self.N + 1)]
    def update(self, histo):
        vals = histo.get()
        length = len(vals)
        assert length == self.N + 1
        self.bins = [self.bins[i] + vals[i] for i in range(length)]


class Counter:
    def __init__(self, name):
        self.name = name
        self.value = 0
        self.count = 0

    def update(self, val):
        self.value += val
        self.count +=1

    def get_val(self):
        return self.value

    def get_count(self):
        return self.count

    def get_mean(self):
        return float(self.value) / self.count

class Stats:
    def __init__(self):
        self.stats = {}
        self.stats["latency"] = Counter("latency")
        self.stats["outstanding"] = Counter("outstanding")

    def update(self, name, val):
        self.stats[name].update(val)

    def get(self, name):
        return self.stats[name]

def getCb(header, payload):
    print "receviced get response"

def genPutMsgs(volId, cnt):
    l = []
    print "Generating put messages"
    for i in range(cnt):
        #data = "This is my put #{}".format(i)
        data = randomword(4096)
        putMsg = FdspUtils.newPutObjectMsg(volId, data)
        l.append(putMsg)
        print "\r%d / %d" % (i, cnt),
    print "Done."
    return l

def genPutMsgs2(volId, cnt):
    rf = RandomFile("/tmp/test", cnt * 4096, 4096)
    l = []
    print "Generating put messages"
    rf.create_file()
    i = 1
    for data in rf.get_object():
        putMsg = FdspUtils.newPutObjectMsg(volId, data)
        l.append(putMsg)
        print "\r%d / %d" % (i, cnt),
        i += 1
    print "Done."
    return l


def main():
    parser = OptionParser()
    parser.add_option("-t", "--threads", dest = "threads", type = "int", default = 1, help = "Number of threads")
    # FIXME: specify total number of request
    parser.add_option("-n", "--num-requests", dest = "n_reqs", type = "int", default = 1,
                      help = "Number of requests per volume")
    parser.add_option("-p", "--port", dest = "port", type = "int", default = 5678, help = "PlatSvc port")
    parser.add_option("-u", "--uuid", dest = "uuid", type = "int", default = 64,
                      help = "Sm uuid for PlatSvc. Must be a power of 2")
    parser.add_option("-o", "--outstanding", dest = "outstanding", type = "int", default = 100,
                      help = "Max number of outstanding requests to SM")

    (options, args) = parser.parse_args()
    cnt = options.n_reqs

    # This script generates bunch of puts and gets against SM
    process.setup_logger()
    p = PlatSvc.PlatSvc(basePort=options.port, mySvcUuid=options.uuid)

    # Get uuids of all sms
    smUuids = p.svcMap.svcUuids('sm')
    # Get volId for volume1
    volId = p.svcMap.omConfig().getVolumeId("volume0")
    # Generate put object message
    putMsgs = genPutMsgs2(volId, cnt)

    #cProfile.run(run_test(options, smUuids, volId, putMsgs, p), filename="profile")
    run_test(options, smUuids, volId, putMsgs, p)

def run_test(options, smUuids, volId, putMsgs, p):
    timeHisto = Histogram(20, 100)
    time_start = time.time()
    cnt = options.n_reqs
    counter = [cnt]
    print "start_time: ", time_start

    tracker = {}
    stats = Stats()

    def putCb(header, payload):
        #print "header_id:",header.msg_src_id,
        elapsed = time.time() - tracker[header.msg_src_id]
        timeHisto.add(elapsed * 1e3)
        stats.update("latency", elapsed)
        #print "time:", elapsed
        counter[0] -= 1
        if counter[0] == 0:
            time_end = time.time()
            print "receviced all put responses", time_end, cnt/(time_end - time_start)

    not_issued_reqs = cnt
    i = 0
    while not_issued_reqs > 0:
        outs = counter[0] - not_issued_reqs
        stats.update("outstanding", outs)

        # print "outs:", outs
        #divider = 500
        # max_outs = int(min(options.outstanding * (i + divider) / divider, 50))
        max_outs = options.outstanding
        if outs < max_outs:
            #print "msg", hex(putMsgs[i].data_obj_id.digest)
            #print "msg:", putMsgs[i]
            tracker[i + 1] = time.time()
            p.sendAsyncSvcReq(smUuids[0], putMsgs[i], putCb)
            not_issued_reqs -= 1
            i += 1
        #else:
            #print "wait"
            #time.sleep(100/1e6)

    # # do gets on the objects that were put
    # for i in range(cnt):
    #     getMsg = FdspUtils.newGetObjectMsg(volId, putMsgs[i].data_obj_id.digest )
    #     p.sendAsyncSvcReq(smUuids[0], getMsg, getCb)

    # wait for kill

    #while True:
    #    time.sleep(1)


    def signal_handler(signal, frame):
        print('... Exiting')
        print "Stats:", stats.stats
        print "Average latency [ms]:", (stats.get("latency").get_mean() * 1e3)
        print "Average outstanding:", (stats.get("outstanding").get_mean())
        print "Latency histogram:", timeHisto.get()
        p.stop()
        sys.exit(0)

    signal.signal(signal.SIGINT, signal_handler)
    print('Press Ctrl+C to exit.')
    signal.pause()

if __name__ == "__main__":
    main()