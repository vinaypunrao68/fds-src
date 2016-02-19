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
import dmtdlt
import argparse

class ConfigDBContext(Context):
    def __init__(self, *args):
        Context.__init__(self, *args)
        self.config.setVolumeApi(self)
        self.__rdb = redis.Redis(port=int(self.config.getSystem(helpers.KEY_REDISPORT)))

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

    #--------------------------------------------------------------------------------------
    @clidebugcmd
    @arg('domain', help= "domian id/name", default = 0, nargs='?')
    def nodes(self, domain):
        'show the node info system'
        try:
            nodes = self.redisDB().smembers('{}:cluster:nodes'.format(domain))
            for node in nodes:
                nodeinfo=self.redisDB().get('node:{}'.format(node))
                pprint.pprint(FdspUtils.deserialize('FDSP_RegisterNodeType',nodeinfo))
                serviceinfo=self.redisDB().get('{}:services'.format(node))
                pprint.pprint(serviceinfo)
                #pprint.pprint(FdspUtils.deserialize('NodeServices',serviceinfo))
        except ApiException, e:
            print e
        except Exception, e:
            log.exception(e)

    #--------------------------------------------------------------------------------------
    @clidebugcmd
    def svcmap(self):
        'show the node info system'
        try:
            services = self.redisDB().hgetall('svcmap')
            for serviceid, serviceinfo in services.items():
                pprint.pprint(serviceid)
                pprint.pprint(FdspUtils.deserialize('SvcInfo',serviceinfo))
        except ApiException, e:
            print e
        except Exception, e:
            log.exception(e)

    #--------------------------------------------------------------------------------------
    @clidebugcmd
    @arg('--domain', help= "domain id/name", default=0)
    def dmt(self, domain=0):
        'show dmt'
        try:
            dmtver= self.redisDB().get('{}:dmt:committed'.format(domain))
            dmtdata=binascii.unhexlify(self.redisDB().get('{}:dmt:{}'.format(domain,dmtver)))
            dmt = dmtdlt.DMT()
            pprint.pprint(dmtdata)
            dmt.load(dmtdata)
            dmt.dump()
        except ApiException, e:
            print e
        except Exception, e:
            log.exception(e)

    #--------------------------------------------------------------------------------------
    @clidebugcmd
    @arg('volumes', help= "vol ids to display", nargs=argparse.REMAINDER)
    @arg('--domain', help= "domain id/name", default = 0)
    def volumes(self, volumes, domain=0):
        'show volumes'
        try:
            volids = sorted(list(self.redisDB().smembers('{}:volumes'.format(domain))), key=lambda x: int(x))
            if len(volumes) > 0:
                volids = [volid for volid in volids if volid in volumes]
            volume = self.redisDB().hgetall('vol:{}'.format(volids[0]))
            allkeys= sorted(volume.keys())
            alldata=[]
            specificids=[]
            count = 1
            for vol in volids:
                volume = self.redisDB().hgetall('vol:{}'.format(vol))
                data=[]
                for n in range(0,len(allkeys)):
                    data.append(volume.get(allkeys[n],''))
                alldata.append(data)
                specificids.append(vol)
                if count % 5 == 0:
                    alldata.insert(0, allkeys)
                    print tabulate(zip(*alldata), headers=['key'] + specificids, tablefmt=self.config.getTableFormat())
                    print
                    alldata=[]
                    specificids=[]
                count += 1

            if len(alldata) > 0 :
                alldata.insert(0, allkeys)
                print tabulate(zip(*alldata),headers=['key'] + specificids, tablefmt=self.config.getTableFormat())
        except ApiException, e:
            print e
        except Exception, e:
            log.exception(e)



