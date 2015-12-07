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
