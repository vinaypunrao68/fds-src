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
    @arg('volid', help= "volume id", type=str)
    @arg('amuuid', help= "am uuid", type=str)
    def handlestate(self, volid, amuuid):
        'gets the volume group handle state from AM'
        volid = self.config.getVolumeApi().getVolumeId(volid)
        for uuid in self.config.getServiceApi().getServiceIds(amuuid):
            print('-->From service {}: '.format(uuid))
            try:
                stateStr = ServiceMap.client(uuid).getStateInfo('volumegrouphandle.{}'.format(volid))
                state = json.loads(stateStr)
                print (json.dumps(state, indent=2, sort_keys=True))
            except Exception, e:
                print "Failed to fetch state.  Either service is down or argument is incorrect"
        return

    #--------------------------------------------------------------------------------------
    @clidebugcmd
    @arg('volid', help= "volume id", type=str)
    @arg('dmuuid', help= "dm uuid", type=str)
    def replicastate(self, volid, dmuuid):
        'gets the volume replica state from DM'
        volid = self.config.getVolumeApi().getVolumeId(volid)
        for uuid in self.config.getServiceApi().getServiceIds(dmuuid):
            print('-->From service {}: '.format(uuid))
            try:
                stateStr = ServiceMap.client(uuid).getStateInfo('volume.{}'.format(volid))
                state = json.loads(stateStr)
                print (json.dumps(state, indent=2, sort_keys=True))
            except Exception, e:
                print "Failed to fetch state.  Either service is down or argument is incorrect"
        return

    #--------------------------------------------------------------------------------------
    @clidebugcmd
    @arg('volid', help= "volume id", type=str)
    @arg('dmuuid', help= "dm uuid", type=str)
    def forcesync(self, volid, dmuuid):
        'Forces sync on a given volume.  Volume must be offline for this to work'
        volid = self.config.getVolumeApi().getVolumeId(volid)
        svc = self.config.getPlatform();
        msg = FdspUtils.newSvcMsgByTypeId('DbgForceVolumeSyncMsg');
        msg.volId = volid
        for uuid in self.config.getServiceApi().getServiceIds(dmuuid):
            cb = WaitedCallback();
            svc.sendAsyncSvcReq(uuid, msg, cb)

            print('-->From service {}: '.format(uuid))
            if not cb.wait(30):
                print 'Failed to force sync: {}'.format(self.config.getServiceApi().getServiceName(uuid))
            elif cb.header.msg_code != 0:
                print 'Failed to force sync error: {}'.format(cb.header.msg_code)
            else:
                print "Initiated force sync"

