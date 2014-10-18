from  svchelper import *
from fdslib.pyfdsp.apis import ttypes
from fds_service.ttypes import *
import platformservice
from platformservice import *
import FdspUtils 


class ScavengerContext(Context):
    def __init__(self, *args):
        Context.__init__(self, *args)

        self.smClient = self.config.platform

    #--------------------------------------------------------------------------------------
    @cliadmincmd
    def enable(self):
        try: 
            smUuids = self.smClient.svcMap.svcUuids('sm')
            getScavMsg = FdspUtils.newEnableScavengerMsg()
            scavCB = WaitedCallback()
            self.smClient.sendAsyncSvcReq(smUuids[0], getScavMsg, scavCB)

        except Exception, e:
            log.exception(e)
            return 'Enable failed'
    

    #--------------------------------------------------------------------------------------
    @cliadmincmd
    def disable(self):
        try: 
            smUuids = self.smClient.svcMap.svcUuids('sm')
            getScavMsg = FdspUtils.newDisableScavengerMsg()
            scavCB = WaitedCallback()
            self.smClient.sendAsyncSvcReq(smUuids[0], getScavMsg, scavCB)
        except Exception, e:
            log.exception(e)
            return 'disable failed'
    
    #--------------------------------------------------------------------------------------
    @cliadmincmd
    def start(self):
        try: 
            smUuids = self.smClient.svcMap.svcUuids('sm')
            getScavMsg = FdspUtils.newStartScavengerMsg()
            scavCB = WaitedCallback()
            self.smClient.sendAsyncSvcReq(smUuids[0], getScavMsg, scavCB)
        except Exception, e:
            log.exception(e)
            return 'start failed'
    
    #--------------------------------------------------------------------------------------
    @cliadmincmd
    def stop(self):
        try: 
            smUuids = self.smClient.svcMap.svcUuids('sm')
            getScavMsg = FdspUtils.newStopScavengerMsg()
            scavCB = WaitedCallback()
            self.smClient.sendAsyncSvcReq(smUuids[0], getScavMsg, scavCB)
        except Exception, e:
            log.exception(e)
            return 'stop failed'


    #--------------------------------------------------------------------------------------

    @cliadmincmd
    def status(self):
        try:
            smUuids = self.smClient.svcMap.svcUuids('sm')
            getStatusMsg = FdspUtils.newScavengerStatusMsg()
            scavCB = WaitedCallback()
            self.smClient.sendAsyncSvcReq(smUuids[0], getStatusMsg, scavCB)
            scavCB.wait()
            resp = scavCB.payload.status
            print "Scavenger status: ",
            if resp == 1:
                print "ACTIVE"
            elif resp == 2:
                print "INACTIVE"
            elif resp == 3:
                print "DISABLED"

        except Exception, e:
            log.exception(e)
            return 'get status failed'


    #--------------------------------------------------------------------------------------
    @cliadmincmd
    def progress(self):
        try:
            smUuids = self.smClient.svcMap.svcUuids('sm')
            getStatusMsg = FdspUtils.newScavengerProgressMsg()
            scavCB = WaitedCallback()
            self.smClient.sendAsyncSvcReq(smUuids[0], getStatusMsg, scavCB)
            scavCB.wait()
            resp = scavCB.payload.progress_pct
            print "Scavenger progress: {}%".format(10)
        except Exception, e:
            log.exception(e)
            return 'get progress failed'

