from svchelper import *
from svc_api.ttypes import *
from common.ttypes import *
from platformservice import *
import FdspUtils
import struct
import pdb


class DMDebugContext(Context):
    def __init__(self, *args):
        Context.__init__(self, *args)

    def dmClient(self):
        return self.config.getPlatform()
    #--------------------------------------------------------------------------------------

    @clicmd
    @arg('vol-name', help= "-Volume name  of the checker")
    @arg('source-dm', help= "-service uuid of the source  DM")
    @arg('dry-run', help= "-Action based on the  checker result, True->sync the nodes False->report the diffs" )
    def dmChk(self, vol_name, source_dm, dry_run ):
        try:
            volume_id  = ServiceMap.omConfig().getVolumeId(vol_name)
            dmChecker = FdspUtils.newDmchkMsg(volume_id, source_dm, dry_run)
            self.dmClient().sendAsyncSvcReq(dm, dmChecker, None)
        except ApiException, e:
            print e
        except Exception, e:
            log.exception(e)
        return 'Error running the dm checker: {}'.format(vol_name)

    #--------------------------------------------------------------------------------------

    @clicmd
    @arg('vol-name', help= "-Volume name of the volume to be copied")
    @arg('source-dm', help= "-service uuid of the source  DM")
    @arg('dest-dm', help= "-service uuid of the destination DM" )
    @arg('archive', help= "Add archive of old dm volume on dest DM")
    def volumeCopy(self, vol_name, source_dm, dest_dm , archive):
        try:
            volume_id  = ServiceMap.omConfig().getVolumeId(vol_name)
            dest_dm = int(dest_dm)
            copyVolume = FdspUtils.newCopyVolumeMsg(volume_id, dest_dm, archive)
            cb = WaitedCallback()
            self.dmClient().sendAsyncSvcReq(int(source_dm), copyVolume, cb)
            print('-->From service {}: '.format(source_dm))
            if not cb.wait(30):
                print 'Failed to force copy'
            elif cb.header.msg_code != 0:
                print 'Failed to force sync error: {}'.format(cb.header.msg_code)
            else:
                print "Completed Volume {} copy from {} to {}".format(vol_name, source_dm, dest_dm)
        except ApiException, e:
            print e
        except Exception, e:
            log.exception(e)
            return 'Error running the volume copy: {}'.format(vol_name)
