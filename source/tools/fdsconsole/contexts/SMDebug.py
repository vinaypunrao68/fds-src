from svchelper import *
from svc_api.ttypes import *
from common.ttypes import *
from platformservice import *
import FdspUtils
import argparse
import itertools
import json

class SMDebugContext(Context):
    def __init__(self, *args):
        Context.__init__(self, *args)

    def smClient(self):
        return self.config.getPlatform()
    #--------------------------------------------------------------------------------------
    @clicmd
    @arg('svcid', help= "-Uuid of the SM/DM/AM to send the command to", type=long)
    @arg('svcname', help= "service name",  choices=['sm','dm','am'])
    def shutdown(self, svcid, svcname):
        'shutdown a service'
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
    @clidebugcmd
    @arg('sm', help="-Uuid of the SM to send the command to", type=long)
    @arg('tokens', help="-List of tokens to check", nargs=argparse.REMAINDER)
    def startSmchk(self, sm, tokens):
        """
        Start the online smchk for the specified sm node
        """
        try:
            if len(tokens) :
                l=[n for n in map(helpers.expandIntRange,tokens)]
                tokens = list(itertools.chain.from_iterable(l))
            checkMsg = FdspUtils.newStartSmchkMsg(tokens)
            self.smClient().sendAsyncSvcReq(sm, checkMsg, None)
        except Exception as e:
            log.exception(e)
            print e.message
            return 'Start online smchk failed'

    #--------------------------------------------------------------------------------------
    @clidebugcmd
    @arg('svcid', help= "Service Uuid",  type=long)
    def showtierstats(self, svcid):
        'display tiering stats'
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

    #--------------------------------------------------------------------------------------
    @clidebugcmd
    @arg('sm', help= "-Uuid of the SM to send the command to", type=long)
    def diskmapchange(self, sm):
        """
        Debug message for sending disk map change notification to SM.
        """
        try:
            dskmpmsg = FdspUtils.newNotifyDiskMapChangeMsg()
            self.smClient().sendAsyncSvcReq(sm, dskmpmsg, None)
        except Exception, e:
            log.exception(e)
            return 'send disk map change message failed'

    #--------------------------------------------------------------------------------------
    @clidebugcmd
    @arg('sm', help= "Uuid of the SM to send the command to", type=long)
    def MigrationTokenCheck(self, sm):
        """
        Check the state of SM tokens (Available or Unavailable) in migration manager
        """
        try:
            state = ServiceMap.client(sm).getStateInfo('migrationmgr')
            state = json.loads(state)
            print (json.dumps(state, indent=2, sort_keys=True)) 
        except Exception, e:
            log.exception(e)
            print e.message
            return 'SM migration token state check failed'

