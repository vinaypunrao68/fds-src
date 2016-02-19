from  svchelper import *
from svc_api.ttypes import *
import platformservice
from platformservice import *
import FdspUtils

class ScavengerPolicyContext(Context):
    '''
    Class to encapsulate methods associated with scavenger polcies.
    '''

    def __init__(self, *args):
        Context.__init__(self, *args)

    def smClient(self):
        return self.config.getPlatform()

    #--------------------------------------------------------------------------------------
    @clicmd
    @arg('threshold1', help= "-Threshold1", type=int)
    @arg('threshold2', help= "-Threshold2", type=int)
    @arg('token-reclaim-threshold', help= "-Token reclaim threshold" , type=int)
    @arg('tokens-per-disk', help="-Tokens per disk", type=int)
    def set(self, threshold1, threshold2, token_reclaim_threshold, tokens_per_disk):
        'set a scavenger policy'
        try: 
            smUuids = self.smClient().svcMap.svcUuids('sm')
            setScavMsg = FdspUtils.newSetScavengerPolicyMsg(threshold1, threshold2, 
                                                            token_reclaim_threshold, tokens_per_disk)
            scavCB = WaitedCallback()
            self.smClient().sendAsyncSvcReq(smUuids[0], setScavMsg, scavCB)
            scavCB.wait()

        except Exception, e:
            log.exception(e)
            return 'set scavenger policy failed'

    @clicmd
    def get(self):
        'get the scavenger policy'
        try: 
            smUuids = self.smClient().svcMap.svcUuids('sm')
            getScavMsg = FdspUtils.newQueryScavengerPolicyMsg()
            scavCB = WaitedCallback()
            self.smClient().sendAsyncSvcReq(smUuids[0], getScavMsg, scavCB)
            scavCB.wait()
            resp = """Current policy:
            Threshold 1: {}
            Threshold 2: {}
            Token reclaim threshold: {}
            Concurrent token compaction per disk: {}""".format(
                scavCB.payload.dsk_threshold1, scavCB.payload.dsk_threshold2,
                scavCB.payload.token_reclaim_threshold, scavCB.payload.tokens_per_dsk)
            print resp
        except Exception, e:
            log.exception(e)
            return 'get scavenger policy failed'

