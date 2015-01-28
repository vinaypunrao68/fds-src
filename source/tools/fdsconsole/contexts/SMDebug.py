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
    @arg('nodeid', help= "-Uuid of the SM/DM to send the command to", type=long)
    @arg('svcname', help= "service name",  choices=['sm','dm'])
    def shutdown(self, nodeid, svcname):
        try:
	    svcUuid = self.smClient.svcMap.svcUuid(nodeid, svcname)
            shutdownMsg = FdspUtils.newShutdownMODMsg()
            self.smClient.sendAsyncSvcReq(svcUuid, shutdownMsg, None)
        except Exception, e:
            log.exception(e)
            return 'Enable failed'
