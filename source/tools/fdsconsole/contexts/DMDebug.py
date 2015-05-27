from svchelper import *
from svc_api.ttypes import *
from common.ttypes import *
from platformservice import *
import FdspUtils
import pdb


class DMDebugContext(Context):
    def __init__(self, *args):
        Context.__init__(self, *args)

    def dmClient(self):
        return self.config.getPlatform()
    #--------------------------------------------------------------------------------------

    @cliadmincmd
    @arg('vol-name', help= "-Volume name  of the checker")
    @arg('dest-dm', help= "-service uuid of the destination DM")
    @arg('chk-remedy', help= "-Action based on the  checker result, True->sync the nodes False->report the diffs" )
    def dmChk(self, vol_name, dest_dm, chk_remedy ):
        try:
            volume_id  = ServiceMap.omConfig().getVolumeId(vol_name)
            dmChecker = FdspUtils.newDmchkMsg(volume_id, dest_dm, chk_remedy)
            self.dmClient().sendAsyncSvcReq(dm, dmChecker, None)
        except ApiException, e:
            print e
        except Exception, e:
            log.exception(e)
        return 'Error running the dm checker: {}'.format(vol_name)
