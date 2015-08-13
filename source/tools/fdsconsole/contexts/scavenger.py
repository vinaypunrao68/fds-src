from  svchelper import *
from svc_api.ttypes import *
import platformservice
from platformservice import *
import FdspUtils


class ScavengerContext(Context):
    def __init__(self, *args):
        Context.__init__(self, *args)

    def smClient(self):
        return self.config.getPlatform()

    #--------------------------------------------------------------------------------------
    @cliadmincmd
    @arg('sm', help= "-Uuid of the SM to send the command to", type=long)
    def enable(self, sm):
        try:
            getScavMsg = FdspUtils.newEnableScavengerMsg()
            scavCB = WaitedCallback()
            self.smClient().sendAsyncSvcReq(sm, getScavMsg, scavCB)

        except Exception, e:
            log.exception(e)
            return 'Enable failed'

    #--------------------------------------------------------------------------------------
    @cliadmincmd
    @arg('sm', help= "-Uuid of the SM to send the command to", type=long)
    def disable(self, sm):
        try:
            getScavMsg = FdspUtils.newDisableScavengerMsg()
            scavCB = WaitedCallback()
            self.smClient().sendAsyncSvcReq(sm, getScavMsg, scavCB)
        except Exception, e:
            log.exception(e)
            return 'disable failed'
    
    #--------------------------------------------------------------------------------------
    @cliadmincmd
    @arg('sm', help= "-Uuid of the SM to send the command to", type=long)
    def start(self, sm):
        try:
            getScavMsg = FdspUtils.newStartScavengerMsg()
            scavCB = WaitedCallback()
            self.smClient().sendAsyncSvcReq(sm, getScavMsg, scavCB)
        except Exception, e:
            log.exception(e)
            return 'start failed'
    
    #--------------------------------------------------------------------------------------
    @cliadmincmd
    @arg('sm', help= "-Uuid of the SM to send the command to", type=long)
    def stop(self, sm):
        try:
            getScavMsg = FdspUtils.newStopScavengerMsg()
            scavCB = WaitedCallback()
            self.smClient().sendAsyncSvcReq(sm, getScavMsg, scavCB)
        except Exception, e:
            log.exception(e)
            return 'stop failed'


    #--------------------------------------------------------------------------------------

    @cliadmincmd
    @arg('sm', help= "-Uuid of the SM to send the command to", type=long)
    def status(self, sm):
        try:
            getStatusMsg = FdspUtils.newScavengerStatusMsg()
            scavCB = WaitedCallback()
            self.smClient().sendAsyncSvcReq(sm, getStatusMsg, scavCB)
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
            getStatusMsg = FdspUtils.newScavengerProgressMsg()
            scavCB = WaitedCallback()
            self.smClient().sendAsyncSvcReq(sm, getStatusMsg, scavCB)
            scavCB.wait()
            resp = scavCB.payload.progress_pct
            print "Scavenger progress: {}%".format(resp)
        except Exception, e:
            log.exception(e)
            return 'get progress failed'


class ScrubberContext(Context):
    def __init__(self, *args):
        Context.__init__(self, *args)

    def smClient(self):
        return self.config.getPlatform()

    @cliadmincmd
    @arg('sm', help= "-Uuid of the SM to send the command to", type=long)
    def enable(self, sm):
        try:
            scrubEnableMsg = FdspUtils.newEnableScrubberMsg()
            scrubCB = WaitedCallback()
            self.smClient().sendAsyncSvcReq(sm, scrubEnableMsg, None)
        except Exception, e:
            print e
            log.exception(e)
            return 'enable scrubber failed'

    @cliadmincmd
    @arg('sm', help= "-Uuid of the SM to send the command to", type=long)
    def disable(self, sm):
        try:
            scrubDisableMsg = FdspUtils.newDisableScrubberMsg()
            scrubCB = WaitedCallback()
            self.smClient().sendAsyncSvcReq(sm, scrubDisableMsg, scrubCB)
        except Exception, e:
            log.exception(e)
            return 'disable scrubber failed'

    @cliadmincmd
    @arg('sm', help= "-Uuid of the SM to send the command to", type=long)
    def status(self, sm):
        try:
            scrubStatusMsg = FdspUtils.newQueryScrubberStatusMsg()
            scrubCB = WaitedCallback()
            self.smClient().sendAsyncSvcReq(sm, scrubStatusMsg, scrubCB)
            scrubCB.wait()
            resp = scrubCB.payload.scrubber_status
            print "Scrubber status: ",
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
            return 'scrubber status failed'


