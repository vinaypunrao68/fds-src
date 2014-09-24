from  svchelper import *
from fdslib.pyfdsp.apis import ttypes

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


