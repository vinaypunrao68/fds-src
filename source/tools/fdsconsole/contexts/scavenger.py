from  svchelper import *
from svc_api.ttypes import *
from svc_types.ttypes import FDSPMsgTypeId
import platformservice
from platformservice import *
import FdspUtils
import humanize

class ScavengerContext(Context):
    def __init__(self, *args):
        Context.__init__(self, *args)

    def smClient(self):
        return self.config.getPlatform()

    #--------------------------------------------------------------------------------------
    @cliadmincmd
    @arg('sm', help= "-Uuid of the SM to send the command to", type=str, default='sm', nargs='?')
    def enable(self, sm):
        try:
            for uuid in self.config.getServiceId(sm, False):
                print 'enabling scavenger on sm:{}'.format(uuid)
                getScavMsg = FdspUtils.newEnableScavengerMsg()
                scavCB = WaitedCallback()
                self.smClient().sendAsyncSvcReq(uuid, getScavMsg, scavCB)

        except Exception, e:
            log.exception(e)
            return 'Enable failed'

    #--------------------------------------------------------------------------------------
    @cliadmincmd
    @arg('sm', help= "-Uuid of the SM to send the command to", type=str, default='sm', nargs='?')
    def disable(self, sm):
        try:
            for uuid in self.config.getServiceId(sm, False):
                print 'disabling scavenger on sm:{}'.format(uuid)
                getScavMsg = FdspUtils.newDisableScavengerMsg()
                scavCB = WaitedCallback()
                self.smClient().sendAsyncSvcReq(uuid, getScavMsg, scavCB)
        except Exception, e:
            log.exception(e)
            return 'disable failed'

    #--------------------------------------------------------------------------------------
    @cliadmincmd
    @arg('sm', help= "-Uuid of the SM to send the command to", type=str, default='sm', nargs='?')
    def start(self, sm):
        try:
            for uuid in self.config.getServiceId(sm, False):
                print 'starting scavenger on sm:{}'.format(uuid)
                getScavMsg = FdspUtils.newStartScavengerMsg()
                scavCB = WaitedCallback()
                self.smClient().sendAsyncSvcReq(uuid, getScavMsg, scavCB)
        except Exception, e:
            log.exception(e)
            return 'start failed'

    #--------------------------------------------------------------------------------------
    @cliadmincmd
    @arg('dm', help= "-Uuid of the DM to send the command to", type=str, default='dm', nargs='?')
    def refscan(self, dm):
        try:
            for uuid in self.config.getServiceId(dm, False):
                print 'starting refscan on dm:{}'.format(uuid)
                msg = FdspUtils.newSvcMsgByTypeId(FDSPMsgTypeId.StartRefScanMsgTypeId)
                cb = WaitedCallback()
                self.smClient().sendAsyncSvcReq(uuid, msg, cb)
        except Exception, e:
            log.exception(e)
            return 'start refscan failed'

    #--------------------------------------------------------------------------------------
    @cliadmincmd
    @arg('sm', help= "-Uuid of the SM to send the command to", type=str, default='sm', nargs='?')
    def stop(self, sm):
        try:
            for uuid in self.config.getServiceId(sm, False):
                print 'stopping scavenger on sm:{}'.format(uuid)
                getScavMsg = FdspUtils.newStopScavengerMsg()
                scavCB = WaitedCallback()
                self.smClient().sendAsyncSvcReq(uuid, getScavMsg, scavCB)
        except Exception, e:
            log.exception(e)
            return 'stop failed'


    #--------------------------------------------------------------------------------------

    @cliadmincmd
    @arg('sm', help= "-Uuid of the SM to send the command to", type=str, default='sm', nargs='?')
    def info(self, sm):
        try:
            cluster_totalobjects = 0
            cluster_deletedobjects = 0
            numsvcs=0
            for uuid in self.config.getServiceId(sm, False):
                numsvcs += 1
                cntrs = ServiceMap.client(uuid).getCounters('*')
                keys=cntrs.keys()
                totalobjects =0
                deletedobjects=0
                totaltokens=0
                for key in keys:
                    if key.find('scavenger.token') >= 0:
                        if key.endswith('.total'):
                            totalobjects += cntrs[key]
                            totaltokens += 1
                        elif key.endswith('.deleted'):
                            deletedobjects += cntrs[key]
                data = []
                gcstart='not yet'
                key = 'sm.scavenger.start.timestamp'
                if key in cntrs and cntrs[key] > 0:
                    gcstart='{} ago'.format(humanize.naturaldelta(time.time()-int(cntrs[key])))

                cluster_totalobjects += totalobjects
                cluster_deletedobjects += deletedobjects
                data.append(('gc.start',gcstart))
                data.append(('num.gc.running', cntrs.get('sm.scavenger.running',0) ))
                data.append(('num.compactors', cntrs.get('sm.scavenger.compactor.running',0)))
                data.append(('objects.total',totalobjects))
                data.append(('objects.deleted',deletedobjects))
                data.append(('tokens.total',totaltokens))
                print ('{}\ngc info for {}:{}\n{}'.format('-'*40, 'sm', uuid, '-'*40))
                print tabulate(data,headers=['key', 'value'], tablefmt=self.config.getTableFormat())

            if numsvcs > 1:
                data =[]
                data.append(('objects.total',cluster_totalobjects))
                data.append(('objects.deleted',cluster_deletedobjects))
                print ('{}\ngc info for the cluster\n{}'.format('-'*40,'-'*40))
                print tabulate(data,headers=['key', 'value'], tablefmt=self.config.getTableFormat())

        except Exception, e:
            log.exception(e)
            return 'get counters failed'


    #--------------------------------------------------------------------------------------
    @cliadmincmd
    @arg('sm', help= "-Uuid of the SM to send the command to", type=str, default='sm', nargs='?')
    def progress(self, sm):
        try:
            for uuid in self.config.getServiceId(sm, False):
                print 'progress of scavenger on sm:{}'.format(uuid)
                getStatusMsg = FdspUtils.newScavengerProgressMsg()
                scavCB = WaitedCallback()
                self.smClient().sendAsyncSvcReq(uuid, getStatusMsg, scavCB)
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
