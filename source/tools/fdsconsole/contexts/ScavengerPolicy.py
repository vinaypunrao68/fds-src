from  svchelper import *
from fdslib.pyfdsp.apis import ttypes
from fds_service.ttypes import *
import platformservice
from platformservice import *
import FdspUtils 

class ScavengerPolicyContext(Context):
    '''
    Class to encapsulate methods associated with scavenger polcies.
    '''

    def __init__(self, *args):
        Context.__init__(self, *args)
        self.smClient = self.config.platform

    #--------------------------------------------------------------------------------------
    @cliadmincmd
    @arg('threshold1', help= "-Threshold1", type=int)
    @arg('threshold2', help= "-Threshold2", type=int)
    @arg('token-reclaim-threshold', help= "-Token reclaim threshold" , type=int)
    @arg('tokens-per-disk', help="-Tokens per disk", type=int)
    def set(self):
        try: 
            smUuids = self.smClient.svcMap.svcUuids('sm')
            getScavMsg = FdspUtils.newSetScavengerPolicyMsg()
            scavCB = WaitedCallback()
            self.smClient.sendAsyncSvcReq(smUuids[0], getScavMsg, scavCB)
        except Exception, e:
            log.exception(e)
            return 'set scavenger policy failed'


    @cliadmincmd
    def get(self):
        try: 
            smUuids = self.smClient.svcMap.svcUuids('sm')
            getScavMsg = FdspUtils.newQueryScavengerPolicyMsg()
            scavCB = WaitedCallback()
            self.smClient.sendAsyncSvcReq(smUuids[0], getScavMsg, scavCB)
        except Exception, e:
            log.exception(e)
            return 'get scavenger policy failed'

