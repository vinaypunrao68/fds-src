from  svchelper import *
from svc_api.ttypes import *
from platformservice import *
import FdspUtils
import restendpoint
import re
import types
import humanize
import time
import socket

class ServiceContext(Context):
    def __init__(self, *args):
        Context.__init__(self, *args)
        self.__restApi = None
        self.config.setServiceApi(self)
        self.serviceList = []
        self.serviceFetchTime = 0

    def restApi(self):
        if self.__restApi == None:
            self.__restApi = restendpoint.ServiceEndpoint(self.config.getRestApi())
        return self.__restApi

    def getServiceList(self, forceFetch=False):
        curTime = int(time.time())
        if (curTime - self.serviceFetchTime) < 10 and len(self.serviceList) > 0 and not forceFetch:
            return self.serviceList
        self.serviceList = self.restApi().listServices()
        self.serviceFetchTime = int(time.time())
        return self.serviceList

    #--------------------------------------------------------------------------------------
    @clicmd
    @arg('nodeid', help= "node id",  type=long)
    @arg('svcname', help= "service name. Must be some combination of sm, dm, or am, separated by commas.")
    def addService(self, nodeid, svcname):
        'activate services on a node'

        # Split it up
        svcname = svcname.split(',')
        # Strip spaces, make lowercase
        svcname = map(lambda x: x.strip().lower(), svcname)

        svcs = {}
        if svcname.count('am') > 0:
            svcs['am'] = True
        if svcname.count('sm') > 0:
            svcs['sm'] = True
        if svcname.count('dm') > 0:
            svcs['dm'] = True

        try:
            return self.restApi().startService(nodeid, svcs)
        except Exception, e:
            log.exception(e)
            return 'unable to start service'

    #--------------------------------------------------------------------------------------
    @clicmd
    @arg('nodeid', help= "node id",  type=long)
    @arg('svcname', help= "service name",  choices=['sm','dm','am'])
    def removeService(self, nodeid, svcname):
        'deactivate services on a node'
        try:
            return self.restApi().toggleServices(nodeid, {svcname: False})
        except Exception, e:
            log.exception(e)
            return 'unable to remove node'

    #--------------------------------------------------------------------------------------
    @clidebugcmd
    @arg('svcid', help= "service id",  type=str, default='*', nargs='?')
    def list(self, svcid):
        'list of services in the system'
        try:
            services = self.getServiceList()
            matchingServices = self.getServiceIds(svcid)
            if matchingServices!= None and len(matchingServices) > 0:
                services = [s for s in services if int(s['uuid']) in matchingServices]
            return tabulate(map(lambda x: [x['uuid'], x['service'], x['ip'], x['port'], x['status']], services),
                            headers=['UUID', 'Service Name', 'IP4 Address', 'TCP Port', 'Service Status'],
                            tablefmt=self.config.getTableFormat())
        except Exception, e:
            log.exception(e)
            return 'unable to get service list'

    #--------------------------------------------------------------------------------------
    # will get service name from uuid
    def getServiceName(self, uuid):
        uuid = str(uuid)
        services = self.getServiceList()
        for s in services:
            if uuid == s['uuid']:
                return '{}:{}'.format(s['service'].lower(), s['ip'])
        raise Exception('unknown service : {}'.format(uuid))

    #--------------------------------------------------------------------------------------
    # will get service type from uuid
    def getServiceType(self, uuid):
        uuid = str(uuid)
        services = self.getServiceList()
        for s in services:
            if uuid == s['uuid']:
                return s['service'].lower()
        raise Exception('unknown service : {}'.format(uuid))

    #--------------------------------------------------------------------------------------
    # will get service id for sm,am on a single node cluster by name
    # match : uuid, app, app:port, app:ip, app:hostname
    #       : hostname, ip
    #       : hostname:port , ip:port

    def getServiceIds(self, pattern):
        return self.getServiceId(pattern , False)
    def getServiceId(self, pattern, onlyone=True):
        if type(pattern) == types.IntType:
            return pattern if onlyone else [pattern]
        if pattern.isdigit():
            return int(pattern) if onlyone else [int(pattern)]

        ipPattern = re.compile("\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}")
        try:
            name=None
            ip=[]
            port=None
            if ':' in pattern:
                name,ipdata = pattern.split(':')
                if ipdata.isdigit():
                    port=int(ipdata)
                    ipdata=None
                elif not ipPattern.match(ipdata):
                    # maybe a hostname
                    try:
                        ip = socket.gethostbyname_ex(ipdata)[2]
                    except:
                        pass
                if len(ip) == 0 and ipdata:
                    ip = [ipdata]
            else:
                name=pattern.lower()

            # check for ip match
            if name and ipPattern.match(name):
                ip = [name]
                name = None

            if name not in ['om','am','pm','sm','dm', '*', 'all', None]:
                # check for hostname
                try:
                    iplist = socket.gethostbyname_ex(name)[2]
                    ip = iplist
                    name = None
                except:
                    pass
                if name != None:
                    print 'unknown service : {}'.format(name)
                    return []

            services = self.getServiceList()
            finallist = [s for s in services
                         if (name == None or name == 'all' or name == '*' or name == s['service'].lower()) and
                         (len(ip) == 0 or s['ip'] in ip) and
                         (port == None or s['port'] == port)]
            if onlyone:
                if len(finallist) == 1:
                    return int(finallist[0]['uuid'])
                else:
                    print 'More than 1 matching services for pattern : {}'.format(pattern)
                    print tabulate(map(lambda x: [x['uuid'], x['service'], x['ip'], x['port'], x['status']], finallist),
                                   headers=['UUID', 'Service Name', 'IP4 Address', 'TCP Port', 'Service Status'],
                                   tablefmt=self.config.getTableFormat())
            else:
                if len(finallist) >= 1:
                    return [int(s['uuid']) for s in finallist]
            print 'No matching services for pattern : {}'.format(pattern)
        except Exception, e:
            log.exception(e)
            return 'unable to get service list'
        return None

    #--------------------------------------------------------------------------------------
    @clicmd
    @arg('svcid', help= "service uuid", type=str)
    @arg('match', help= "regex pattern", type=str, default=None, nargs='?')
    @arg('-z','--zero', help= "show zeros", action='store_true', default=False)
    def listcounter(self, svcid, match, zero=False):
        'list debug counters'
        try:
            for uuid in self.getServiceIds(svcid):
                p=helpers.get_simple_re(match)
                cntrs = ServiceMap.client(uuid).getCounters('*')
                addeditems={}
                for key in cntrs:
                    if not zero and cntrs[key] == 0:
                        continue
                    if key.endswith('.timestamp'):
                        value='{} ago'.format(humanize.naturaldelta(time.time()-int(cntrs[key]))) if cntrs[key] > 0 else 'not yet'
                        addeditems[key + ".human"] = value
                    elif key.endswith('.totaltime'):
                        value = '{}'.format(humanize.naturaldelta(cntrs[key]))
                        addeditems[key + ".human"] = value

                cntrs.update(addeditems)

                data = [(v,k) for k,v in cntrs.iteritems() if (not p or p.match(k)) and (zero or v != 0)]
                data.sort(key=itemgetter(1))
                if len(data) > 0:
                    print ('{}\ncounters for {}\n{}'.format('-'*40, self.getServiceName(uuid), '-'*40))
                    print tabulate(data,headers=['value', 'counter'], tablefmt=self.config.getTableFormat())

        except Exception, e:
            log.exception(e)
            return 'unable to get counter'

    #--------------------------------------------------------------------------------------
    @clidebugcmd
    @arg('svcid', help= "service Uuid",  type=str)
    @arg('match', help= "regex pattern",  type=str, default=None, nargs='?')
    def listconfig(self, svcid, match):
        'list the config values of a service'
        try:
            for uuid in self.getServiceIds(svcid):
                p=helpers.get_simple_re(match)
                cntrs = ServiceMap.client(uuid).getConfig(0)
                data = [(k.lower(),v) for k,v in cntrs.iteritems() if not p or p.match(k)]
                data.sort(key=itemgetter(0))
                if len(data) > 0:
                    print ('{}\nconfig set for {}\n{}'.format('-'*40, self.getServiceName(uuid), '-'*40))
                    print (tabulate(data,headers=['config', 'value'], tablefmt=self.config.getTableFormat()))
        except Exception, e:
            log.exception(e)
            return 'unable to get config list'


    #--------------------------------------------------------------------------------------
    @clidebugcmd
    @arg('svcid', help= "service Uuid",  type=str)
    @arg('key'  , help= "config key"  ,  type=str)
    @arg('value', help= "config value",  type=str)
    def setconfig(self, svcid, key, value):
        'set a config value'
        try:
            for uuid in self.getServiceIds(svcid):
                configmap = ServiceMap.client(uuid).getConfig(0)
                oldvalue = configmap.get(key, '')
                ServiceMap.client(uuid).setConfigVal(key, value)
                print 'set config [{}] from [{}] to [{}] for [{}]'.format(key, oldvalue, value, self.getServiceName(uuid))
        except Exception, e:
            log.exception(e)
            return 'unable to set config'

    #--------------------------------------------------------------------------------------
    @clidebugcmd
    @arg('svcid', help= "service Uuid",  type=str)
    @arg('value', help= "config value",  type=str, choices=['error','warn','normal','debug','trace','critical'])
    def setloglevel(self, svcid, value):
        'set the logging level - error/warn/normal/debug/trace/critical'
        try:
            for uuid in self.getServiceIds(svcid):
                name=self.getServiceType(uuid)
                key='fds.{}.log_severity'.format(name)
                self.setconfig(uuid, key, value)
        except Exception, e:
            log.exception(e)
            return 'unable to set log level'

    #--------------------------------------------------------------------------------------
    @clidebugcmd
    @arg('svcid', help= "Service Uuid",  type=str)
    def showsvcmap(self, svcid):
        'display the service map'
        try:
            svcid=self.getServiceId(svcid)
            svcMap = ServiceMap.client(svcid).getSvcMap(None)
            data = [(e.svc_id.svc_uuid.svc_uuid, e.incarnationNo, e.svc_status, e.ip, e.svc_port) for e in svcMap]
            data.sort(key=itemgetter(0))
            return tabulate(data,headers=['uuid', 'incarnation', 'status', 'ip', 'port'], tablefmt=self.config.getTableFormat())

        except Exception, e:
            log.exception(e)
            return 'unable to get svcmap'

    #--------------------------------------------------------------------------------------
    @clidebugcmd
    @arg('svcid', help= "Service Uuid",  type=str)
    def listflag(self, svcid, name=None):
        'show flags for the service'
        try:
            svcid=self.getServiceId(svcid)
            if name is None:
                flags = ServiceMap.client(svcid).getFlags(None)
                data = [(v,k) for k,v in flags.iteritems()]
                data.sort(key=itemgetter(1))
                return tabulate(data, headers=['value', 'flag'], tablefmt=self.config.getTableFormat())
            else:
                return ServiceMap.client(svcid).getFlag(name)
        except Exception, e:
            log.exception(e)
            return 'unable to get volume list'

    #--------------------------------------------------------------------------------------
    @clidebugcmd
    @arg('svcid', type=str)
    @arg('flag', type=str)
    @arg('value', type=long)
    def setflag(self, svcid, flag, value):
        'set the flag for a service'
        svcid=self.getServiceId(svcid)
        try:
            ServiceMap.client(svcid).setFlag(flag, value)
        except Exception, e:
            log.exception(e)
            return 'Unable to set flag: {}'.format(flag)
    #--------------------------------------------------------------------------------------
    @clidebugcmd
    @arg('svcid', type=str)
    @arg('cmd', type=str)
    def setfault(self, svcid, cmd):
        'set the specified fault'
        svcid=self.getServiceId(svcid)
        try:
            success = ServiceMap.client(svcid).setFault(cmd)
            if success:
                return 'Ok'
            else:
                return "Failed to inject fault.  Check the command"
        except Exception, e:
            log.exception(e)
            return 'Unable to set fault'

    #--------------------------------------------------------------------------------------
    @clidebugcmd
    @arg('volname', help='-volume name')
    def showvolumestats(self, volname):
        'display info about no. of objects/blobs'
        try:
            data = []
            dmClient = self.config.getPlatform();

            dmUuids = dmClient.svcMap.svcUuids('dm')
            volId = dmClient.svcMap.omConfig().getVolumeId(volname)

            getblobmeta = FdspUtils.newGetVolumeMetaDataMsg(volId);
            cb = WaitedCallback();
            dmClient.sendAsyncSvcReq(dmUuids[0], getblobmeta, cb)

            if not cb.wait():
                print 'async volume meta request failed'
            else:
                data += [("numblobs",cb.payload.volume_meta_data.blobCount)]
                data += [("size",cb.payload.volume_meta_data.size)]
                data += [("numobjects",cb.payload.volume_meta_data.objectCount)]


            getstatsmsg = FdspUtils.newGetDmStatsMsg(volId);
            statscb = WaitedCallback();
            dmClient.sendAsyncSvcReq(dmUuids[0], getstatsmsg, statscb)

            if not statscb.wait():
                print 'async get dm stats request failed'
            else:
                data += [("commitlogsize",statscb.payload.commitlog_size)]
                data += [("extent0size",statscb.payload.extent0_size)]
                data += [("extentsize",statscb.payload.extent_size)]
                data += [("metadatasize",statscb.payload.metadata_size)]

            print tabulate(data, tablefmt=self.config.getTableFormat())
        except Exception, e:
            log.exception(e)
            return 'unable to get volume meta'
