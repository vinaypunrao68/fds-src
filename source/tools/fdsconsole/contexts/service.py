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
    @arg('nodeid', type=long)
    @arg('svcname', help= "service name",  choices=['sm','dm','am','om'])
    @arg('cmd', type=str)
    def setfault(self, nodeid, svcname, cmd):
        try:
            success = ServiceMap.client(nodeid, svcname).setFault(cmd)
            if success:
                return 'Ok'
            else:
                return "Failed to inject fault.  Check the command"
        except Exception, e:
            log.exception(e)
            return 'Unable to set fault'

    #--------------------------------------------------------------------------------------
    @clicmd
    @arg('volname', help='-volume name')
    def listblobstat(self, volname):
        try:
            
            #process.setup_logger()
	    # import pdb; pdb.set_trace()
            dmClient = self.config.platform;

            dmUuids = dmClient.svcMap.svcUuids('dm')
            volId = dmClient.svcMap.omConfig().getVolumeId(volname)

            getblobmeta = FdspUtils.newGetVolumeMetaDataMsg(volId);
            cb = WaitedCallback();
            dmClient.sendAsyncSvcReq(dmUuids[0], getblobmeta, cb)

            if not cb.wait():
		print 'async volume meta request failed'

    	    data = []
	    data += [("numblobs",cb.payload.volume_meta_data.blobCount)]
	    data += [("size",cb.payload.volume_meta_data.size)]
	    data += [("numobjects",cb.payload.volume_meta_data.objectCount)]
	    print tabulate(data, tablefmt=self.config.getTableFormat())


            getblobmsg = FdspUtils.newGetBucketMsg(volId, 0, 0);
            listcb = WaitedCallback();
            dmClient.sendAsyncSvcReq(dmUuids[0], getblobmsg, listcb)

            if not listcb.wait():
		print 'async listblob request failed'

	    #import pdb; pdb.set_trace()
            blobs = listcb.payload.blob_info_list;
            blobs.sort(key=attrgetter('blob_name'))
            return tabulate([(x.blob_name, x.blob_size, x.mime_type) for x in blobs],headers=
                 ['blobname', 'blobsize', 'blobtype'], tablefmt=self.config.getTableFormat())

        except Exception, e:
            log.exception(e)
            return 'unable to get volume meta list'

    #--------------------------------------------------------------------------------------
    @clicmd
    @arg('volname', help='-volume name')
    def listdmstats(self, volname):
        try:
            
            #process.setup_logger()
	    # import pdb; pdb.set_trace()
            dmClient = self.config.platform;
            volId = dmClient.svcMap.omConfig().getVolumeId(volname)

            dmUuids = dmClient.svcMap.svcUuids('dm')
            getstatsmsg = FdspUtils.newGetDmStatsMsg(volId);
            statscb = WaitedCallback();
            dmClient.sendAsyncSvcReq(dmUuids[0], getstatsmsg, statscb)

            if not statscb.wait():
		print 'async get dm stats request failed'

    	    data = []
	    data += [("commitlogsize",statscb.payload.commitlog_size)]
	    data += [("extent0size",statscb.payload.extent0_size)]
	    data += [("extentsize",statscb.payload.extent_size)]
	    data += [("metadatasize",statscb.payload.metadata_size)]
	    return  tabulate(data, tablefmt=self.config.getTableFormat())

        except Exception, e:
            print e
            log.exception(e)
            return 'unable to get dm stats '

    #--------------------------------------------------------------------------------------
    @clicmd
    @arg('volname', help='-volume name')
    @arg('pattern', help='-blob name pattern for search')
    @arg('maxkeys', help= "-max number for keys", nargs='?' , type=long, default=1000)
    @arg('startpos', help= "-starting  position of the blob list", nargs='?' , type=long, default=0)
    def listhdfsblob(self, volname, pattern, maxkeys, startpos):
        try:
            
            #process.setup_logger()
	    # import pdb; pdb.set_trace()
            dmClient = self.config.platform;

            dmUuids = dmClient.svcMap.svcUuids('dm')
            volId = dmClient.svcMap.omConfig().getVolumeId(volname)

            getbloblist = FdspUtils.newListBlobsByPatternMsg(volId, startpos, maxkeys, pattern);
            cb = WaitedCallback();
            dmClient.sendAsyncSvcReq(dmUuids[0], getbloblist, cb)

            if not cb.wait():
		print 'async listblob request failed'

	    #import pdb; pdb.set_trace()
            blobs = listcb.payload.blobDescriptors;
            blobs.sort(key=attrgetter('blob_name'))
            return tabulate([(x.blob_name, x.blob_size, x.mime_type) for x in blobs],headers=
                 ['blobname', 'blobsize', 'blobtype'], tablefmt=self.config.getTableFormat())

        except Exception, e:
            log.exception(e)
            return 'unable to get blob list based on pattern '

