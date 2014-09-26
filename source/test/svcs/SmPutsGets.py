import time
import sys
sys.path.append('../fdslib/')
sys.path.append('../fdslib/pyfdsp/')
from fds_service.ttypes import *
import platformservice
import FdspUtils
import process

def putCb(header, payload):
    print "receviced put response"

def getCb(header, payload):
    print "receviced get response"

def genPutMsgs(volId, cnt):
    l = []
    for i in range(cnt):
        data = "This is my put #{}".format(i)
        putMsg = FdspUtils.newPutObjectMsg(volId, data)
        l.append(putMsg)
    return l

# This script generates bunch of puts and gets against SM
process.setup_logger()
p = platformservice.PlatSvc(basePort=5678)

cnt = 10
# Get uuids of all sms
smUuids = p.svcMap.svcUuids('sm')
# Get volId for volume1
volId = p.svcMap.omConfig().getVolumeId("volume1")
# Generate put object message
putMsgs = genPutMsgs(volId, cnt)
# do a bunch of puts
for i in range(cnt):
    p.sendAsyncSvcReq(smUuids[0], putMsgs[i], putCb)
# do gets on the objects that were put
for i in range(cnt):
    getMsg = FdspUtils.newGetObjectMsg(volId, putMsgs[i].data_obj_id.digest )
    p.sendAsyncSvcReq(smUuids[0], getMsg, getCb)
# wait for kill
while True:
    time.sleep(1)
p.stop()
