from  svchelper import *
from fdslib.pyfdsp.apis import ttypes
from fds_service.ttypes import *
import platformservice
from platformservice import *
import FdspUtils 


class ScavengerContext(Context):
    def __init__(self, *args):
        Context.__init__(self, *args)

    #--------------------------------------------------------------------------------------
    @clicmd
    def enable(self):
        try: 
            smClient = self.config.platform;
            smUuids = smClient.svcMap.svcUuids('sm')
            getScavMsg = FdspUtils.newEnableScavengerMsg()
            scavCB = WaitedCallback()
            smClient.sendAsyncSvcReq(smUuids[0], getScavMsg, scavCB)

        except Exception, e:
            log.exception(e)
            return 'Enable failed'
    

    #--------------------------------------------------------------------------------------
    @clicmd
    def disable(self):
        try: 
            smClient = self.config.platform;
            smUuids = smClient.svcMap.svcUuids('sm')
            getScavMsg = FdspUtils.newDisableScavengerMsg()
            scavCB = WaitedCallback()
            smClient.sendAsyncSvcReq(smUuids[0], getScavMsg, scavCB)
        except Exception, e:
            log.exception(e)
            return 'disable failed'
    
    #--------------------------------------------------------------------------------------
    @clicmd
    def start(self):
        try: 
            smClient = self.config.platform;
            smUuids = smClient.svcMap.svcUuids('sm')
            getScavMsg = FdspUtils.newStartScavengerMsg()
            scavCB = WaitedCallback()
            smClient.sendAsyncSvcReq(smUuids[0], getScavMsg, scavCB)
        except Exception, e:
            log.exception(e)
            return 'start failed'
    
    #--------------------------------------------------------------------------------------
    @clicmd
    def stop(self):
        try: 
            smClient = self.config.platform;
            smUuids = smClient.svcMap.svcUuids('sm')
            getScavMsg = FdspUtils.newStopScavengerMsg()
            scavCB = WaitedCallback()
            smClient.sendAsyncSvcReq(smUuids[0], getScavMsg, scavCB)
        except Exception, e:
            log.exception(e)
            return 'stop failed'


    
    
    #--------------------------------------------------------------------------------------
