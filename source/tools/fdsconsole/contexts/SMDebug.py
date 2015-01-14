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

    #--------------------------------------------------------------------------------------
    @clidebugcmd
    @arg('nodeid', help= "-Uuid of the SM to send the command to", type=long)
    def startHtc(self, nodeid):
        """
        Debug message for starting hybrid tier controller.
        NOTE: Once long term tiering design is consolidated, this message should go into
        tiering context
        """
        try:
            sm = self.smClient.svcMap.svcUuid(nodeid, "sm")
            startHtc = FdspUtils.newCtrlStartHybridTierCtrlrMsg()
            self.smClient.sendAsyncSvcReq(sm, startHtc, None)
        except Exception, e:
            log.exception(e)
            return 'Enable failed'

    #--------------------------------------------------------------------------------------
    @clicmd
    @arg('nodeid', help= "node id",  type=long)
    def listtierstats(self, nodeid):
        try:
            cntrs = ServiceMap.client(nodeid, 'sm').getCounters('*')
            data = [('hdd-reads', cntrs['hdd_reads:volume=0']),
                    ('hdd-writes', cntrs['hdd_writes:volume=0']),
                    ('ssd-reads', cntrs['ssd_reads:volume=0']),
                    ('ssd-writes', cntrs['ssd_writes:volume=0']),
                    ('hybrid moved', cntrs['movedCnt:volume=0'])]
            return tabulate(data,headers=['counter', 'value'], tablefmt=self.config.getTableFormat())
            
        except Exception, e:
            log.exception(e)
            return 'unable to get tier stats'
