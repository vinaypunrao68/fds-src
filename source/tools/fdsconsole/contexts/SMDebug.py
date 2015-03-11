from  svchelper import *
from svc_api.ttypes import *
import platformservice
from platformservice import *
import FdspUtils 


class SMDebugContext(Context):
    def __init__(self, *args):
        Context.__init__(self, *args)

    def smClient(self):
        return self.config.getPlatform()
    #--------------------------------------------------------------------------------------
    @cliadmincmd
    @arg('svcid', help= "-Uuid of the SM/DM/AM to send the command to", type=long)
    @arg('svcname', help= "service name",  choices=['sm','dm','am'])
    def shutdown(self, svcid, svcname):
        try:
            shutdownMsg = FdspUtils.newShutdownMODMsg()
            self.smClient().sendAsyncSvcReq(svcid, shutdownMsg, None)
        except Exception, e:
            log.exception(e)
            return 'Shutdown failed'

    #--------------------------------------------------------------------------------------
    @clidebugcmd
    @arg('sm', help= "-Uuid of the SM to send the command to", type=long)
    def startHtc(self, sm):
        """
        Debug message for starting hybrid tier controller.
        NOTE: Once long term tiering design is consolidated, this message should go into
        tiering context
        """
        try:
            startHtc = FdspUtils.newCtrlStartHybridTierCtrlrMsg()
            self.smClient().sendAsyncSvcReq(sm, startHtc, None)
        except Exception, e:
            log.exception(e)
            return 'Enable failed'

    #--------------------------------------------------------------------------------------
    @clicmd
    @arg('svcid', help= "Service Uuid",  type=long)
    def listtierstats(self, svcid):
        try:
            cntrs = ServiceMap.client(svcid).getCounters('*')
            data = [('hdd-reads', cntrs['hdd_reads:volume=0']),
                    ('hdd-writes', cntrs['hdd_writes:volume=0']),
                    ('ssd-reads', cntrs['ssd_reads:volume=0']),
                    ('ssd-writes', cntrs['ssd_writes:volume=0']),
                    ('hybrid moved', cntrs['movedCnt:volume=0'])]
            return tabulate(data,headers=['counter', 'value'], tablefmt=self.config.getTableFormat())
            
        except Exception, e:
            log.exception(e)
            return 'unable to get tier stats'
