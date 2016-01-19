from svchelper import *
from svc_types.ttypes import *
from common.ttypes import *
from platformservice import *
import FdspUtils
import json


class VolumeGroupContext(Context):
    def __init__(self, *args):
        Context.__init__(self, *args)

    #--------------------------------------------------------------------------------------
    @clidebugcmd
    @arg('volid', help= "volume id", type=long)
    @arg('amuuid', help= "am uuid", type=str)
    def handlestate(self, volid, amuuid):
        'gets the volume group handle state from AM'
        for uuid in self.config.getServiceApi().getServiceIds(amuuid):
            stateStr = ServiceMap.client(uuid).getStateInfo('volumegrouphandle.{}'.format(volid))
            state = json.loads(stateStr)
            print (json.dumps(state, indent=2))
        return

    #--------------------------------------------------------------------------------------
    @clidebugcmd
    @arg('volid', help= "volume id", type=long)
    @arg('dmuuid', help= "dm uuid", type=str)
    def replicastate(self, volid, dmuuid):
        'gets the volume replica state from DM'
        for uuid in self.config.getServiceApi().getServiceIds(dmuuid):
            stateStr = ServiceMap.client(uuid).getStateInfo('volume.{}'.format(volid))
            state = json.loads(stateStr)
            print (json.dumps(state, indent=2))
        return

    #--------------------------------------------------------------------------------------
    @clidebugcmd
    @arg('volid', help= "volume id", type=long)
    @arg('dmuuid', help= "dm uuid", type=str)
    def forcesync(self, volid, dmuuid):
        'Forces sync on a given volume.  Volume must be offline for this to work'
        svc = self.config.getPlatform();
        msg = FdspUtils.newSvcMsgByTypeId('DbgForceVolumeSyncMsg');
        msg.volId = volid
        for uuid in self.config.getServiceApi().getServiceIds(dmuuid):
            cb = WaitedCallback();
            svc.sendAsyncSvcReq(uuid, msg, cb)

            if not cb.wait(30):
                print 'Failed to force sync: {}'.format(self.config.getServiceApi().getServiceName(uuid))
            elif cb.header.msg_code != 0:
                print 'Failed to force sync error: {}'.format(cb.header.msg_code)
            else:
                print "Initiated force sync"

