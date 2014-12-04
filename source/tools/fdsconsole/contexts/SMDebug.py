from  svchelper import *
from fdslib.pyfdsp.apis import ttypes
from fds_service.ttypes import *
import platformservice
from platformservice import *
import FdspUtils 


class SMDebugContext(Context):
    def __init__(self, *args):
        Context.__init__(self, *args)

        self.smClient = self.config.platform

    #--------------------------------------------------------------------------------------
    @cliadmincmd
    @arg('sm', help= "-Uuid of the SM to send the command to", type=long)
    def shutdown(self, sm):
        try:
            sm = self.smClient.svcMap.svcUuid(sm, "sm")
            shutdownMsg = FdspUtils.newShutdownSMMsg()
            self.smClient.sendAsyncSvcReq(sm, shutdownMsg, None)
        except Exception, e:
            log.exception(e)
            return 'Enable failed'
