from  svchelper import *
from svc_api.ttypes import *
from svc_types.ttypes import FDSPMsgTypeId
import platformservice
from platformservice import *
import FdspUtils

class ScavengerContext(Context):
    def __init__(self, *args):
        Context.__init__(self, *args)

    def smClient(self):
        return self.config.getPlatform()

    #--------------------------------------------------------------------------------------
    @clidebugcmd
    @arg('sm', help= "-Uuid of the SM to send the command to", type=str, default='sm', nargs='?')
    def enable(self, sm):
        'enable garbage collection'
        try:
            for uuid in self.config.getServiceApi().getServiceIds(sm):
                print 'enabling scavenger on {}'.format(self.config.getServiceApi().getServiceName(uuid))
                getScavMsg = FdspUtils.newEnableScavengerMsg()
                scavCB = WaitedCallback()
                self.smClient().sendAsyncSvcReq(uuid, getScavMsg, scavCB)

        except Exception, e:
            log.exception(e)
            return 'Enable failed'

    #--------------------------------------------------------------------------------------
    @clidebugcmd
    @arg('sm', help= "-Uuid of the SM to send the command to", type=str, default='sm', nargs='?')
    def disable(self, sm):
        'disable garbage collection'
        try:
            for uuid in self.config.getServiceApi().getServiceIds(sm):
                print 'disabling scavenger on {}'.format(self.config.getServiceApi().getServiceName(uuid))
                getScavMsg = FdspUtils.newDisableScavengerMsg()
                scavCB = WaitedCallback()
                self.smClient().sendAsyncSvcReq(uuid, getScavMsg, scavCB)
        except Exception, e:
            log.exception(e)
            return 'disable failed'

    #--------------------------------------------------------------------------------------
    @clidebugcmd
    @arg('sm', help= "-Uuid of the SM to send the command to", type=str, default='sm', nargs='?')
    def startscavenger(self, sm, force=False):
        'start garbage collection on sm'
        try:
            for uuid in self.config.getServiceApi().getServiceIds(sm):
                print 'starting scavenger on {}'.format(self.config.getServiceApi().getServiceName(uuid))
                getScavMsg = FdspUtils.newStartScavengerMsg()
                scavCB = WaitedCallback()
                self.smClient().sendAsyncSvcReq(uuid, getScavMsg, scavCB)
        except Exception, e:
            log.exception(e)
            return 'start failed'

    #--------------------------------------------------------------------------------------
    @clidebugcmd
    @arg('svc', help= "DMs id to send the command to", type=str, default='dm', nargs='?')
    def start(self, svc):
        'start gc process'
        try:
            for uuid in self.config.getServiceApi().getServiceIds(svc):
                try:
                    print 'starting expunge on {}'.format(self.config.getServiceApi().getServiceName(uuid))
                    if self.config.getServiceApi().getServiceType(uuid) == 'sm':
                        msg = FdspUtils.newSvcMsgByTypeId(FDSPMsgTypeId.GenericCommandMsgTypeId)
                        msg.command="scavenger.start"
                    else:
                        msg = FdspUtils.newSvcMsgByTypeId(FDSPMsgTypeId.StartRefScanMsgTypeId)
                    cb = WaitedCallback()
                    self.smClient().sendAsyncSvcReq(uuid, msg, cb)
                except:
                    print 'unable to start gc on {}'.format(self.config.getServiceApi().getServiceName(uuid))
        except Exception, e:
            log.exception(e)
            return 'start gc via dm failed'

    #--------------------------------------------------------------------------------------
    @clidebugcmd
    @arg('dm', help= "-Uuid of the DM to send the command to", type=str, default='dm', nargs='?')
    def refscan(self, dm):
        'start object refscanner on dm'
        try:
            for uuid in self.config.getServiceApi().getServiceIds(dm):
                print 'starting refscan on {}'.format(self.config.getServiceApi().getServiceName(uuid))
                msg = FdspUtils.newSvcMsgByTypeId(FDSPMsgTypeId.StartRefScanMsgTypeId)
                cb = WaitedCallback()
                self.smClient().sendAsyncSvcReq(uuid, msg, cb)
        except Exception, e:
            log.exception(e)
            return 'start refscan failed'

    #--------------------------------------------------------------------------------------
    @clidebugcmd
    @arg('sm', help= "-Uuid of the SM to send the command to", type=str, default='sm', nargs='?')
    def stop(self, sm):
        'stop garbage collection'
        try:
            for uuid in self.config.getServiceApi().getServiceIds(sm):
                print 'stopping scavenger on {}'.format(self.config.getServiceApi().getServiceName(uuid))
                getScavMsg = FdspUtils.newStopScavengerMsg()
                scavCB = WaitedCallback()
                self.smClient().sendAsyncSvcReq(uuid, getScavMsg, scavCB)
        except Exception, e:
            log.exception(e)
            return 'stop failed'


    #--------------------------------------------------------------------------------------
    @clidebugcmd
    @arg('--full', help= "show info from individual services", default=False)
    @arg('sm', help= "-Uuid of the SM to send the command to", type=str, default='*', nargs='?')
    def info(self, sm, full=False):
        'show information about garbage collection'
        try:
            gcdata =[]
            cluster_totalobjects = 0
            cluster_deletedobjects = 0
            cluster_inactiveobjects = 0
            cluster_num_gc_running = 0
            cluster_num_compactor_running = 0
            cluster_num_refscan_running = 0
            numsvcs = 0
            dm=False

            if sm == '*' :
                sm='sm'
                dm=True

            for uuid in self.config.getServiceApi().getServiceIds(sm):
                try:
                    numsvcs += 1
                    cntrs = ServiceMap.client(uuid).getCounters('*')
                    helpers.addHumanInfo(cntrs)
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

                    cluster_totalobjects += totalobjects
                    cluster_deletedobjects += deletedobjects
                    cluster_inactiveobjects += cntrs.get('sm.scavenger.inactive.count',0)
                    data.append(('gc.start',cntrs.get('sm.scavenger.start.timestamp.human','not yet')))

                    if cntrs.get('sm.scavenger.running',0) > 0: cluster_num_gc_running += 1
                    if cntrs.get('sm.scavenger.compactor.running',0) : cluster_num_compactor_running += 1

                    data.append(('num.gc.running', cntrs.get('sm.scavenger.running',0) ))
                    data.append(('gc.run.count', cntrs.get('sm.scavenger.run.count',0) ))
                    data.append(('num.compactors', cntrs.get('sm.scavenger.compactor.running',0)))
                    data.append(('objects.total',totalobjects))
                    data.append(('objects.deleted',deletedobjects))
                    data.append(('tokens.total',totaltokens))
                    gcdata.append(('{}.info'.format(self.config.getServiceApi().getServiceName(uuid)),
                                   '{} : {}'.format(cntrs.get('sm.scavenger.run.count',0), cntrs.get('sm.scavenger.start.timestamp.human','not yet')) ))
                    if full:
                        print ('{}\ngc info for {}\n{}'.format('-'*40, self.config.getServiceApi().getServiceName(uuid), '-'*40))
                        print tabulate(data,headers=['key', 'value'], tablefmt=self.config.getTableFormat())
                except:
                    print 'unable to get data from {}'.format(self.config.getServiceApi().getServiceName(uuid))

            gcdata.append(('',''))
            gcdata.append(('sm.objects.total',cluster_totalobjects))
            gcdata.append(('sm.objects.deleted',cluster_deletedobjects))
            #gcdata.append(('sm.objects.inactive',cluster_inactiveobjects))
            gcdata.append(('sm.doing.gc',cluster_num_gc_running))
            gcdata.append(('sm.doing.compaction',cluster_num_compactor_running))
            gcdata.append(('',''))
            if dm:
                totalobjects =0
                totalvolumes=0
                for uuid in self.config.getServiceApi().getServiceIds('dm'):
                    try:
                        cntrs = ServiceMap.client(uuid).getCounters('*')
                        keys=cntrs.keys()
                        helpers.addHumanInfo(cntrs)
                        data = []

                        data.append(('dm.refscan.lastrun',cntrs.get('dm.refscan.lastrun.timestamp.human','not yet')))
                        data.append(('dm.refscan.num_objects', cntrs.get('dm.refscan.num_objects',0)))
                        data.append(('dm.refscan.num_volumes', cntrs.get('dm.refscan.num_volumes',0)))
                        data.append(('dm.refscan.running', cntrs.get('dm.refscan.running',0)))
                        data.append(('dm.refscan.run.count', cntrs.get('dm.refscan.run.count',0)))
                        if cntrs.get('dm.refscan.running',0) > 0 :  cluster_num_refscan_running += 1
                        totalobjects += cntrs.get('dm.refscan.num_objects',0)
                        totalvolumes += cntrs.get('dm.refscan.num_volumes',0)
                        gcdata.append(('{}.info'.format(self.config.getServiceApi().getServiceName(uuid)),
                                       '{} : {}'.format(cntrs.get('dm.refscan.run.count',0), cntrs.get('dm.refscan.lastrun.timestamp.human','not yet')) ))

                        if full:
                            print ('{}\ngc info for {}\n{}'.format('-'*40, self.config.getServiceApi().getServiceName(uuid), '-'*40))
                            print tabulate(data,headers=['key', 'value'], tablefmt=self.config.getTableFormat())
                    except:
                        print 'unable to get data from {}'.format(self.config.getServiceApi().getServiceName(uuid))
                gcdata.append(('',''))
                gcdata.append(('dm.objects.total',totalobjects))
                gcdata.append(('dm.volumes.total',totalvolumes))
                gcdata.append(('dm.doing.refscan',cluster_num_refscan_running))

            print ('\n{}\ncombined gc info\n{}'.format('='*40,'='*40))
            print tabulate(gcdata,headers=['key', 'value'], tablefmt=self.config.getTableFormat())
            print '=' * 40

        except Exception, e:
            log.exception(e)
            return 'get counters failed'


    #--------------------------------------------------------------------------------------
    @arg('sm', help= "-Uuid of the SM to send the command to", type=str, default='sm', nargs='?')
    def progress(self, sm):
        try:
            for uuid in self.config.getServiceApi().getServiceIds(sm):
                print 'progress of scavenger on {}'.format(self.config.getServiceApi().getServiceName(uuid))
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

    @clidebugcmd
    @arg('sm', help= "-Uuid of the SM to send the command to", type=long)
    def enable(self, sm):
        'enable scrubber'
        try:
            scrubEnableMsg = FdspUtils.newEnableScrubberMsg()
            scrubCB = WaitedCallback()
            self.smClient().sendAsyncSvcReq(sm, scrubEnableMsg, None)
        except Exception, e:
            log.exception(e)
            return 'enable scrubber failed'

    @clidebugcmd
    @arg('sm', help= "-Uuid of the SM to send the command to", type=long)
    def disable(self, sm):
        'disable scrubber'
        try:
            scrubDisableMsg = FdspUtils.newDisableScrubberMsg()
            scrubCB = WaitedCallback()
            self.smClient().sendAsyncSvcReq(sm, scrubDisableMsg, scrubCB)
        except Exception, e:
            log.exception(e)
            return 'disable scrubber failed'

    @clidebugcmd
    @arg('sm', help= "-Uuid of the SM to send the command to", type=long)
    def info(self, sm):
        'show info about scrubber status'
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
