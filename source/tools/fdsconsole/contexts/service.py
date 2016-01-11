from  svchelper import *
from svc_api.ttypes import *
from platformservice import *
import FdspUtils
import restendpoint
import re
import types
import time
import socket

class ServiceContext(Context):
    def __init__(self, *args):
        Context.__init__(self, *args)
        self.__restApi = None
        self.config.setServiceApi(self)
        self.serviceList = []
        self.serviceFetchTime = 0
        self.hostToIpMap = {}
        self.ipToHostMap = {}

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
        pms = [ (s['ip'],s['uuid']) for s in self.serviceList if s['status'] == 'ACTIVE' and s['service'] == 'PM' ]
        for ip,uuid in pms:
            # print ip, self.getHostFromIp(ip), uuid
            if ip == self.getHostFromIp(ip):
                try:
                    name =  ServiceMap.client(int(uuid)).getProperty('hostname')
                    self.hostToIpMap[name] = [ip]
                except Exception as e:
                    pass

        return self.serviceList

    def getIpsFromName(self, name) :
        try:
            if name not in self.hostToIpMap:
                self.hostToIpMap[name] = socket.gethostbyname_ex(name)[2]
            return self.hostToIpMap[name]
        except:
            return [name]

    def getHostFromIp(self, ip) :
        try :
            for name, iplist in self.hostToIpMap.iteritems():
                if ip in iplist:
                    return name
        except:
            return ip
        return ip

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
            return tabulate(map(lambda x: [x['uuid'], x['service'], x['ip'], self.getHostFromIp(x['ip']),  x['port'], x['status']], services),
                            headers=['UUID', 'Type', 'IP', 'Host', 'Port', 'Status'],
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
                host = self.getHostFromIp(s['ip'])
                return '{}@{}'.format(s['service'].lower(), host)
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
                    resolvedIps = self.getIpsFromName(ipdata)
                    if len(resolvedIps) > 0:
                        ip = resolvedIps

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
                iplist = self.getIpsFromName(name)
                if len(iplist) > 0:
                    ip = iplist
                    name = None

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
    @clidebugcmd
    @arg('svcid', help= "service uuid", type=str)
    @arg('match', help= "regex pattern", type=str, default=None, nargs='?')
    @arg('-z','--zero', help= "show zeros", action='store_true', default=False)
    def counters(self, svcid, match, zero=False):
        'list debug counters'
        try:
            for uuid in self.getServiceIds(svcid):
                p=helpers.get_simple_re(match)
                cntrs = ServiceMap.client(uuid).getCounters('*')
                helpers.addHumanInfo(cntrs, not zero)
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
    def configs(self, svcid, match):
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
    def properties(self, svcid):
        'list the properties of a service'
        try:
            for uuid in self.getServiceIds(svcid):
                data = ServiceMap.client(uuid).getProperties(0)
                data = [(k.lower(),v) for k,v in data.iteritems()]
                data.sort(key=itemgetter(0))
                if len(data) > 0:
                    print ('{}\nproperties of for {}\n{}'.format('-'*40, self.getServiceName(uuid), '-'*40))
                    print (tabulate(data,headers=['name', 'value'], tablefmt=self.config.getTableFormat()))
        except Exception, e:
            log.exception(e)
            return 'unable to get properties list'

    #--------------------------------------------------------------------------------------
    @clidebugcmd
    @arg('svcid', help= "service Uuid",  type=str, default='all', nargs='?')
    def buildinfo(self, svcid):
        'list the properties of a service'
        plist=['build.date','build.mode','build.os','build.version']
        infoMap={}

        for uuid in self.getServiceIds(svcid):
            try:
                data = ServiceMap.client(uuid).getProperties(0)
                data = [(k.lower(),v) for k,v in data.iteritems() if k in plist]
                data.sort(key=itemgetter(0))
                key=hash(str(data))
                if key not in infoMap:
                    infoMap[key] = {'data' : data, 'uuidlist' : []}
                infoMap[key]['uuidlist'].append(uuid)
            except Exception, e:
                log.exception(e)
                print 'unable to get build info for {}'.format(self.getServiceName(uuid))

        if len(infoMap) == 0:
            print 'no build info to display'
            return

        if len(infoMap) == 1:
            key=infoMap.keys()[0]
            print '{}\nAll [{}] services seem to have the same build info.\n{}'.format('-'*60, len(infoMap[key]['uuidlist']), '-'*60)
            print tabulate(infoMap[key]['data'],headers=['name', 'value'], tablefmt=self.config.getTableFormat())
        else:
            print '{}\nooops!! differring build info detected.\n{}'.format('-'*40, '-'*40)
            for key in infoMap.keys():
                print '{}\nservices : {}'.format('-'*40, infoMap[key]['uuidlist'])
                print tabulate(infoMap[key]['data'],headers=['name', 'value'], tablefmt=self.config.getTableFormat())

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
            for uuid in self.getServiceIds(svcid):
                print ('\n{}\nsvcmap for {}\n{}'.format('-'*40, self.getServiceName(uuid), '-'*40))
                svcMap = ServiceMap.client(uuid).getSvcMap(None)
                data = [(e.svc_id.svc_uuid.svc_uuid, e.incarnationNo, e.svc_status, e.ip, e.svc_port) for e in svcMap]
                data.sort(key=itemgetter(0))
                print tabulate(data,headers=['uuid', 'incarnation', 'status', 'ip', 'port'], tablefmt=self.config.getTableFormat())
        except Exception, e:
            log.exception(e)
            return 'unable to get svcmap'

    #--------------------------------------------------------------------------------------
    @clidebugcmd
    @arg('svcid', type=str)
    @arg('cmd', type=str)
    def setfault(self, svcid, cmd):
        'set the specified fault'
        for uuid in self.getServiceIds(svcid):
            print ('\n{}\nsvcmap for {}\n{}'.format('-'*40, self.getServiceName(uuid), '-'*40))
            try:
                success = ServiceMap.client(uuid).setFault(cmd)
                if success:
                    print 'Ok'
                else:
                    print "Failed to inject fault.  Check the command"
            except Exception, e:
                log.exception(e)
                print 'Unable to set fault [{}] @ [{}]'.format(cmd,self.getServiceName(uuid))

    #--------------------------------------------------------------------------------------
    @clidebugcmd
    @arg('dm', help= "-Uuid of the DM to send the command to", type=str, default='*', nargs='?')
    def dminfo(self, dm):
        'show information about Data Managers'
        try:
            dmData =[]
            cluster_totalVolMigrated = 0;
            cluster_totalMigrationAborted = 0;
            cluster_totalActiveVolSenders = 0;
            cluster_totalActiveVolReceivers = 0;
            cluster_migrationIsActive = 0;
            cluster_totalOutstandingAsyncMsgs = 0;
            cluster_totalBytesMigrated = 0;
            cluster_totalSecsSpentMigrating = 0;
            cluster_currentDMTVersion = 0;
            if dm == '*' :
                dm='dm'

            for uuid in self.config.getServiceApi().getServiceIds(dm):
                cntrs = ServiceMap.client(uuid).getCounters('*')
                data = []
                data.append(('num.total.ActiveSenders', cntrs.get('dm.migration.active.executors',0)))
                cluster_totalActiveVolReceivers += cntrs.get('dm.migration.active.executors',0)
                data.append(('num.total.ActiveReceivers', cntrs.get('dm.migration.active.clients',0)))
                cluster_totalActiveVolSenders += cntrs.get('dm.migration.active.clients',0)
                data.append(('num.total.ActiveAsyncMsgs', cntrs.get('dm.migration.active.outstandingios',0)))
                cluster_totalOutstandingAsyncMsgs += cntrs.get('dm.migration.active.outstandingios',0)
                data.append(('num.total.AbortedMigrations', cntrs.get('dm.migration.aborted',0)))
                cluster_totalMigrationAborted += cntrs.get('dm.migration.aborted',0)
                data.append(('num.total.VolumesSent', cntrs.get('dm.migration.volumes.sent',0)))
                cluster_totalVolMigrated += cntrs.get('dm.migration.volumes.sent',0)
                data.append(('num.total.VolumesReceived', cntrs.get('dm.migration.volumes.rcvd',0)))
                data.append(('num.totalBytesMigrated', cntrs.get('dm.migration.bytes',0)))
                cluster_totalBytesMigrated += cntrs.get('dm.migrations.bytes',0)
                data.append(('num.total.secSpent', cntrs.get('dm.migration.total.duration',0)))
                cluster_totalSecsSpentMigrating += cntrs.get('dm.migration.total.duration',0)
                print ('{}\ngc info for {}\n{}'.format('-'*40, self.config.getServiceApi().getServiceName(uuid), '-'*40))
                print tabulate(data,headers=['key', 'value'], tablefmt=self.config.getTableFormat())
                if cntrs.get('dm.migration.active.executors',0) > 0 or cntrs.get('dm.migration.active.clients',0) > 0:
                    cluster_migrationIsActive = 1;

            dmData.append(('dm.migration.total.activeSenders',cluster_totalActiveVolSenders))
            dmData.append(('dm.migration.total.activeReceivers',cluster_totalActiveVolReceivers))
            dmData.append(('dm.migration.total.outstandingAsyncMsgs',cluster_totalOutstandingAsyncMsgs))
            dmData.append(('dm.migration.total.migrationAborted',cluster_totalMigrationAborted))
            dmData.append(('dm.migration.total.volSent',cluster_totalVolMigrated))
            dmData.append(('dm.migration.total.bytesMigrated',cluster_totalBytesMigrated))
            dmData.append(('dm.migration.total.SecSpent',cluster_totalSecsSpentMigrating))
            print ('\n{}\ncombined dm info\n{}'.format('='*40,'='*40))
            print tabulate(dmData,headers=['key', 'value'], tablefmt=self.config.getTableFormat())
            print '=' * 40
            if cluster_migrationIsActive > 0:
                print "DM Migration is ACTIVE"
            else:
                print "DM Migration is INACTIVE"

        except Exception, e:
            log.exception(e)
            return 'get counters failed: ' + str(e)
