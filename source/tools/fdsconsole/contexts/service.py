from  svchelper import *
from fdslib.pyfdsp.apis import ttypes
from fds_service.ttypes import *
import platformservice
from platformservice import *
import FdspUtils 

class ServiceContext(Context):
    def __init__(self, *args):
        Context.__init__(self, *args)

    #--------------------------------------------------------------------------------------
    @clicmd
    def list(self):
        try:
            services = ServiceMap.list()
            return tabulate(services, headers=['nodeid','service','ip','port'], tablefmt=self.config.getTableFormat())
        except Exception, e:
            log.exception(e)
            return 'unable to get volume list'

    #--------------------------------------------------------------------------------------
    @clicmd
    @arg('nodeid', help= "node id",  type=long)
    @arg('svcname', help= "service name",  choices=['sm','dm','am','om'])
    def listcounter(self, nodeid, svcname):
        try:
            cntrs = ServiceMap.client(nodeid, svcname).getCounters('*')
            data = [(v,k) for k,v in cntrs.iteritems()]
            data.sort(key=itemgetter(1))
            return tabulate(data,headers=['value', 'counter'], tablefmt=self.config.getTableFormat())
            
        except Exception, e:
            log.exception(e)
            return 'unable to get volume list'

    #--------------------------------------------------------------------------------------
    @clicmd
    @arg('nodeid', help= "node id",  type=long)
    @arg('svcname', help= "service name",  choices=['sm','dm','am','om'])
    def listflag(self, nodeid, svcname, name=None):
        try:
            if name is None:
                flags = ServiceMap.client(nodeid, svcname).getFlags(None)
                data = [(v,k) for k,v in flags.iteritems()]
                data.sort(key=itemgetter(1))
                return tabulate(data, headers=['value', 'flag'], tablefmt=self.config.getTableFormat())
            else:
                return ServiceMap.client(nodeid, svcname).getFlag(name)
        except Exception, e:
            log.exception(e)
            return 'unable to get volume list'

    #--------------------------------------------------------------------------------------
    @clicmd
    @arg('nodeid', type=long)
    @arg('svcname', help= "service name",  choices=['sm','dm','am','om'])
    @arg('flag', type=str)
    @arg('value', type=long)
    def setflag(self, nodeid, svcname, flag, value):
        try:
            ServiceMap.client(nodeid, svcname).setFlag(flag, value)
            return 'Ok'
        except Exception, e:
            log.exception(e)
            return 'Unable to set flag: {}'.format(flag)

    #--------------------------------------------------------------------------------------
    @clicmd
    @arg('nodeid', help= "node id",  type=long)
    @arg('svcname', help= "service name",  choices=['sm','dm','am','om'])
    def listblobstats(self, nodeid, svcname):
        try:
            cntrs = ServiceMap.client(nodeid, svcname).getCounters('*')
            data = [(v,k) for k,v in cntrs.iteritems()]
            data.sort(key=itemgetter(1))
            return tabulate(data,headers=['value', 'counter'], tablefmt=self.config.getTableFormat())
            
        except Exception, e:
            log.exception(e)
            return 'unable to get volume list'

    #--------------------------------------------------------------------------------------
    @clicmd
    @arg('volname', help='-volume name')
    def listblobstat(self, volname):
        try:
            
            process.setup_logger()
	    import pdb; pdb.set_trace()
            dmClient = platformservice.PlatSvc(basePort=5679)

            dmUuids = dmClient.svcMap.svcUuids('dm')
            volId = dmClient.svcMap.omConfig().getVolumeId(volname)

            getblobmeta = FdspUtils.newGetVolumeMetaDataMsg(volId);
            cb = WaitedCallback();
            dmClient.sendAsyncSvcReq(dmUuids[0], getblobmeta, cb)

            if not cb.wait():
		print 'async request failed'

	    print '{}'.cb.payload.volume_meta_data.blobCount;
	    print '{}'.cb.payload.volume_meta_data.size;
	    print '{}'.cb.payload.volume_meta_data.objectCount;
            return 'Success' 
        except Exception, e:
            log.exception(e)
            return 'unable to get volume meta list'

