from svchelper import *
from svc_types.ttypes import *
from common.ttypes import *
from platformservice import *
from restendpoint import *
import humanize
import md5,sha
import os
import FdspUtils
import binascii
import redis
import pprint

class ConfigDBContext(Context):
    def __init__(self, *args):
        Context.__init__(self, *args)
        self.config.setVolumeApi(self)
        self.__rdb = redis.Redis()

    def redisDB(self):
        return self.__rdb

    #--------------------------------------------------------------------------------------
    @clidebugcmd
    @arg('volume', help= "volume id/name")
    def snapshots(self, volume):
        'show the list of snapshots for a volume'
        try:
            volId = self.config.getVolumeApi().getVolumeId(volume)
            snaps = self.redisDB().hgetall('volume:{}:snapshots'.format(volId))
            for snapid, snap in snaps.iteritems():
                pprint.pprint(FdspUtils.deserialize('Snapshot',snap))
        except ApiException, e:
            print e
        except Exception, e:
            log.exception(e)

    #--------------------------------------------------------------------------------------
    @clidebugcmd
    @arg('volume', help= "volume id/name", default = None, nargs='?')
    def snapshotpolicies(self, volume):
        'show the list of snapshot polcies for a volume/ system'
        try:
            policies = self.redisDB().hgetall('snapshot.policies')
            for policyid, policy in policies.iteritems():
                pprint.pprint(FdspUtils.deserialize('SnapshotPolicy',policy))
        except ApiException, e:
            print e
        except Exception, e:
            log.exception(e)

