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
    @arg('sm', help= "-Uuid of the SM to send the command to", type=long)
    def enable(self, sm):
        try:
            sm = self.smClient.svcMap.svcUuid(sm, "sm")

            getScavMsg = FdspUtils.newEnableScavengerMsg()
            scavCB = WaitedCallback()
            self.smClient.sendAsyncSvcReq(sm, getScavMsg, scavCB)

        except Exception, e:
            log.exception(e)
            return 'Enable failed'
    

    #--------------------------------------------------------------------------------------
    @cliadmincmd
    @arg('sm', help= "-Uuid of the SM to send the command to", type=long)
    def disable(self, sm):
        try:
            sm = self.smClient.svcMap.svcUuid(sm, "sm")

            getScavMsg = FdspUtils.newDisableScavengerMsg()
            scavCB = WaitedCallback()
            self.smClient.sendAsyncSvcReq(sm, getScavMsg, scavCB)
        except Exception, e:
            log.exception(e)
            return 'disable failed'
    
    #--------------------------------------------------------------------------------------
    @cliadmincmd
    @arg('sm', help= "-Uuid of the SM to send the command to", type=long)
    def start(self, sm):
        try:
            sm = self.smClient.svcMap.svcUuid(sm, "sm")

            getScavMsg = FdspUtils.newStartScavengerMsg()
            scavCB = WaitedCallback()
            self.smClient.sendAsyncSvcReq(sm, getScavMsg, scavCB)
        except Exception, e:
            log.exception(e)
            return 'start failed'
    
    #--------------------------------------------------------------------------------------
    @cliadmincmd
    @arg('sm', help= "-Uuid of the SM to send the command to", type=long)
    def stop(self, sm):
        try:
            sm = self.smClient.svcMap.svcUuid(sm, "sm")

            getScavMsg = FdspUtils.newStopScavengerMsg()
            scavCB = WaitedCallback()
            self.smClient.sendAsyncSvcReq(sm, getScavMsg, scavCB)
        except Exception, e:
            log.exception(e)
            return 'stop failed'


    #--------------------------------------------------------------------------------------

    @cliadmincmd
    @arg('sm', help= "-Uuid of the SM to send the command to", type=long)
    def status(self, sm):
        try:
            sm = self.smClient.svcMap.svcUuid(sm, "sm")

            getStatusMsg = FdspUtils.newScavengerStatusMsg()
            scavCB = WaitedCallback()
            self.smClient.sendAsyncSvcReq(sm, getStatusMsg, scavCB)
            scavCB.wait()
            resp = scavCB.payload.status
            print "Scavenger status: ",
            if resp == 1:
                print "ACTIVE"
            elif resp == 2:
                print "INACTIVE"
            elif resp == 3:
                print "DISABLED"
            elif resp == 4:
                print "FINISHING"

        except Exception, e:
            log.exception(e)
            return 'get status failed'


    #--------------------------------------------------------------------------------------
    @cliadmincmd
    @arg('sm', help= "-Uuid of the SM to send the command to", type=long)
    def progress(self, sm):
        try:
            sm = self.smClient.svcMap.svcUuid(sm, "sm")

            getStatusMsg = FdspUtils.newScavengerProgressMsg()
            scavCB = WaitedCallback()
            self.smClient.sendAsyncSvcReq(sm, getStatusMsg, scavCB)
            scavCB.wait()
            resp = scavCB.payload.progress_pct
            print "Scavenger progress: {}%".format(resp)
        except Exception, e:
            log.exception(e)
            return 'get progress failed'


class ScrubberContext(Context):
    def __init__(self, *args):
        Context.__init__(self, *args)

        self.smClient = self.config.platform

    @cliadmincmd
    @arg('sm', help= "-Uuid of the SM to send the command to", type=long)
    def enable(self, sm):
        try:
            sm = self.smClient.svcMap.svcUuid(sm, "sm")

            scrubEnableMsg = FdspUtils.newEnableScrubberMsg()
            scrubCB = WaitedCallback()
            self.smClient.sendAsyncSvcReq(sm, scrubEnableMsg, None)
        except Exception, e:
            log.exception(e)
            return 'enable scrubber failed'

    @cliadmincmd
    @arg('sm', help= "-Uuid of the SM to send the command to", type=long)
    def disable(self, sm):
        try:
            sm = self.smClient.svcMap.svcUuid(sm, "sm")

            scrubDisableMsg = FdspUtils.newDisableScrubberMsg()
            scrubCB = WaitedCallback()
            self.smClient.sendAsyncSvcReq(sm, scrubDisableMsg, scrubCB)
        except Exception, e:
            log.exception(e)
            return 'disable scrubber failed'

    @cliadmincmd
    @arg('sm', help= "-Uuid of the SM to send the command to", type=long)
    def status(self, sm):
        try:
            sm = self.smClient.svcMap.svcUuid(sm, "sm")

            scrubStatusMsg = FdspUtils.newQueryScrubberStatusMsg()
            scrubCB = WaitedCallback()
            self.smClient.sendAsyncSvcReq(sm, scrubStatusMsg, None)
        except Exception, e:
            log.exception(e)
            return 'scrubber status failed'


