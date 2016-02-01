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
import dmtdlt

class VolumeContext(Context):
    def __init__(self, *args):
        Context.__init__(self, *args)
        self.config.setVolumeApi(self)
        self.__restApi = None

    def restApi(self):
        if self.__restApi == None:
            self.__restApi = VolumeEndpoint(self.config.getRestApi())
        return self.__restApi

    def s3Api(self):
        return self.config.getS3Api()

    def getVolumeId(self, volume):
        volume=str(volume)
        client = self.config.getPlatform();
        if volume.isdigit():
            volname = client.svcMap.omConfig().getVolumeName(int(volume))
            if len(volname) > 0:
                return int(volume)
        volId = client.svcMap.omConfig().getVolumeId(volume)
        if int(volId) == 0:
            raise Exception('invalid volume: {}'.format(volume))
        return int(volId)

    #--------------------------------------------------------------------------------------
    @clidebugcmd
    @arg('-s','--sysvols', help= "show system volumes", action='store_true', default=False)
    def list(self, sysvols=False):
        'show the list of volumes in the system'
        try:
            volumes = ServiceMap.omConfig().listVolumes("")
            if not sysvols:
                volumes = [v for v in volumes if not v.name.startswith('SYSTEM_')]
            volumes.sort(key=lambda vol: ResourceState._VALUES_TO_NAMES[vol.state] + ':' + vol.name)
            return tabulate([(item.volId, item.name, item.tenantId, item.dateCreated, ResourceState._VALUES_TO_NAMES[item.state],
                              'OBJECT' if item.policy.volumeType == 0 else 'BLOCK',
                              item.policy.maxObjectSizeInBytes, item.policy.blockDeviceSizeInBytes) for item in volumes],
                            headers=['Id','Name', 'TenantId', 'Create Date','State','Type', 'Max-Obj-Size', 'Blk-Size'], tablefmt=self.config.getTableFormat())
        except ApiException, e:
            print e
        except Exception, e:
            log.exception(e)
        return 'unable to get volume list'

    #--------------------------------------------------------------------------------------
    @clicmd
    @arg('vol-name', help= "-Volume name  of the clone")
    @arg('clone-name', help= "-name of  the  volume clone")
    @arg('policy-id', help= "-volume policy id" , default=0, type=int, nargs='?')
    @arg('timeline-time', help= "timeline time  of parent volume", nargs='?' , type=long, default=0)
    def clone(self, vol_name, clone_name, policy_id, timeline_time):
        'clone a given volume'
        try:
            volume_id  = ServiceMap.omConfig().getVolumeId(vol_name)
            ServiceMap.omConfig().cloneVolume(volume_id, policy_id, clone_name, timeline_time)
            return
        except ApiException, e:
            print e
        except Exception, e:
            log.exception(e)
        return 'create clone failed: {}'.format(vol_name)

    #--------------------------------------------------------------------------------------
    @arg('vol-name', help= "-volume name  of the clone")
    @arg('clone-name', help= "-name of  the  volume clone for restore")
    def restore(self, vol_name, clone_name):
        try:
            volume_id  = ServiceMap.omConfig().getVolumeId(vol_name)
            ServiceMap.omConfig().restoreClone(volume_id, clone_name)
            return
        except ApiException, e:
            print e
        except Exception, e:
            log.exception(e)
        return 'restore clone failed: {}'.format(vol_name)


    #--------------------------------------------------------------------------------------
    @clicmd
    @arg('vol-name', help='-volume name')
    @arg('--domain', help='-domain to add volume to')
    @arg('--minimum', help='-qos minimum guarantee', type=int)
    @arg('--maximum', help='-qos maximum', type=int)
    @arg('--priority', help='-qos priority', type=int)
    @arg('--max-obj-size', help='-maxiumum size (in bytes) of volume objects', type=int)
    @arg('--vol-type', help='-type of volume to create', choices=['block','object'])
    @arg('--blk-dev-size', help='-maximum size (in bytes) of block device', type=int)
    @arg('--tenant-id', help='-id of tenant to create volume under', type=int)
    @arg('--commit-log-retention', help= " continuous commit log retention time in seconds", type=long)
    @arg('--media-policy', help='-media policy for volume', choices=['ssd', 'hdd', 'hybrid'])
    def create(self, vol_name, domain='abc', priority=10, minimum=0, maximum=0, max_obj_size=0,
               vol_type='object', blk_dev_size=21474836480, tenant_id=1, commit_log_retention=86400, media_policy='hdd'):
        'create a new volume'
        try:
            res = self.restApi().createVolume(vol_name,
                                            priority,
                                            minimum,
                                            maximum,
                                            vol_type,
                                            blk_dev_size,
                                            'B',
                                            media_policy,
                                            commit_log_retention,
                                            max_obj_size)
            return
        except ApiException, e:
            log.exception(e)
        except Exception, e:
            log.exception(e)

        return 'create volume failed: {}'.format(vol_name)


    #--------------------------------------------------------------------------------------
    @clicmd
    @arg('volume', help='-volume name/id')
    @arg('--minimum', help='-qos minimum guarantee')
    @arg('--maximum', help='-qos maximum')
    @arg('--priority', help='-qos priority')
    @arg('--commit_log_retention', help="journal log retention in seconds")
    def modify(self, volume, minimum=None, maximum=None, priority=None, commit_log_retention=None):
        'modify an existing volume'
        try:
            if minimum == None and maximum==None and priority==None and commit_log_retention==None:
                print 'please specify one of [minimum, maximum, priority, commit_log_retention]'
                return
            vols = self.restApi().listVolumes()
            volId = self.getVolumeId(volume)
            volume = ServiceMap.omConfig().getVolumeName(int(volId))
            thisVol = None
            for vol in vols:
                if vol['name'] == volume:
                    thisVol=vol
                    break

            if thisVol:
                if commit_log_retention != None:
                    thisVol['dataProtectionPolicy']['commitLogRetention']['seconds'] = int(commit_log_retention)
                if priority != None:
                    thisVol['qosPolicy']['priority'] = int(priority)
                if minimum != None:
                    thisVol['qosPolicy']['iopsMin'] = int(minimum)
                if maximum != None:
                    thisVol['qosPolicy']['iopsMax'] = int(maximum)

            res = self.restApi().modify(thisVol)
            print thisVol
            print res
            return
        except ApiException, e:
            log.exception(e)
        except Exception, e:
            log.exception(e)

        print 'modify volume failed: {}'.format(volume)


    #--------------------------------------------------------------------------------------
    @clicmd
    @arg('--domain', help='-name of domain that volume resides in')
    @arg('vol-name', help='-volume name')
    def delete(self, vol_name, domain='abc'):
        'delete an existing volume'
        try:
            ServiceMap.omConfig().deleteVolume(domain, vol_name)
            return
        except ApiException, e:
            print e
        except Exception, e:
            log.exception(e)
        return 'delete volume failed: {}'.format(vol_name)

    #--------------------------------------------------------------------------------------
    @clidebugcmd
    @arg('volname', help='-volume name')
    @arg('pattern', help='-blob name pattern for search', nargs='?', default='')
    @arg('count', help= "-max number for results", nargs='?' , type=long, default=1000)
    @arg('startpos', help= "-starting index of the blob list", nargs='?' , type=long, default=0)
    @arg('orderby', help= "-order by BLOBNAME or BLOBSIZE", nargs='?', default='UNSPECIFIED')
    @arg('descending', help="-display in descending order", nargs='?')
    @arg('patternSemantics', help="-", nargs='?', default='PCRE')
    @arg('delimiter', help="-", nargs='?', default='/')
    def listblobs(self, volname, pattern, count, startpos, orderby, descending, patternSemantics, delimiter):
        'list blobs from a specific volume'
        try:
            dmClient = self.config.getPlatform();

            for uuid in self.config.getServiceApi().getServiceIds('dm'):
                volId = dmClient.svcMap.omConfig().getVolumeId(volname)

                getblobmsg = FdspUtils.newGetBucketMsg(volId, startpos, count);
                getblobmsg.pattern = pattern;
                if orderby == 'BLOBNAME':
                    getblobmsg.orderBy = 1;
                elif orderby == 'BLOBSIZE':
                    getblobmsg.orderBy = 2;
                else:
                    getblobmsg.orderBy = 0;
                getblobmsg.descending = descending;
                cb = WaitedCallback();
                if patternSemantics == 'PCRE':
                    getblobmsg.patternSemantics = 0
                elif patternSemantics == 'PREFIX':
                    getblobmsg.patternSemantics = 1
                elif patternSemantics == 'PREFIX_AND_DELIMITER':
                    getblobmsg.patternSemantics = 2
                getblobmsg.delimiter = delimiter
                dmClient.sendAsyncSvcReq(uuid, getblobmsg, cb)

                if not cb.wait():
                    print 'req failed : {} : error={}'.format(self.config.getServiceApi().getServiceName(uuid), cb.header.msg_code if cb.header!= None else "--")
                    continue

                #import pdb; pdb.set_trace()
                blobs = cb.payload.blob_descr_list;
                # blobs.sort(key=attrgetter('blob_name'))
                print tabulate([(x.name, x.byteCount) for x in blobs],headers=
                     ['blobname', 'blobsize'], tablefmt=self.config.getTableFormat())

        except Exception, e:
            log.exception(e)
            print 'unable to get blob list'

    #--------------------------------------------------------------------------------------
    @clicmd
    @arg('value', help='value' , nargs='?')
    def put(self, vol_name, key, value):
        '''
        put an object into the volume
        to put a file : start key/value with @
        put <volname> @filename  --> key will be name of the file
        put <volname> <key> @filename
        '''
        try:
            if key.startswith('@'):
                value = open(key[1:],'rb').read()
                key = os.path.basename(key[1:])
            elif value != None and value.startswith('@'):
                value = open(value[1:],'rb').read()
            b = self.s3Api().get_bucket(vol_name)
            k = b.new_key(key)
            num = k.set_contents_from_string(value)

            if num >= 0:
                data = []
                data += [('key' , key)]
                data += [('md5sum' , md5.md5(value).hexdigest())]
                data += [('length' , str(len(value)))]
                data += [('begin' , str(value[:30]))]
                data += [('end' , str(value[-30:]))]
                return tabulate(data, tablefmt=self.config.getTableFormat())
            else:
                print "empty data"
        except Exception, e:
            log.exception(e)
            return 'put {} failed on volume: {}'.format(key, vol_name)


    #--------------------------------------------------------------------------------------
    @clicmd
    @arg('volume', help= "vol name/id")
    @arg('count', help= "count of objs to put", type=long, default=10, nargs="?")
    @arg('--seed', help= "seed the key name", default='1')
    def bulkget(self, volume, count, seed='1'):
        '''
        Does bulk gets.
        TODO: Make it do random gets as well
        '''
        for i in xrange(0, count):
            k = "key_{}_{}".format(seed, i)
            print self.get(volume, k)

    #--------------------------------------------------------------------------------------
    @clicmd
    @arg('volume', help= "vol name/id")
    @arg('count', help= "count of objs to put", type=long, default=10, nargs="?")
    @arg('--seed', help= "seed the key name", default='1')
    def bulkput(self, volume, count, seed='1'):
        '''
        Does bulk put
        '''
        for i in xrange(0, count):
            k = "key_{}_{}".format(seed, i)
            v = "value_{}_{}".format(seed, i)
            print self.put(volume, k, v)

    #--------------------------------------------------------------------------------------
    @clicmd
    @arg('volume', help= "vol name/id")
    @arg('count', help= "count of objs to put", type=long, default=10, nargs="?")
    @arg('--seed', help= "seed the key name", default='1')
    def bulkdelete(self, volume, count, seed='1'):
        'bulk delete from a volume'
        try:
            for i in xrange(0, count):
                k = "key_{}_{}".format(seed, i)
                print 'deleting : {}'.format(k)
                self.deleteobject(volume, k)
        except Exception, e:
            log.exception(e)
            return 'delete failed on volume: {}'.format(volume)

    #--------------------------------------------------------------------------------------
    @clidebugcmd
    def get(self, vol_name, key):
        'get an object from the volume'
        try:
            b = self.s3Api().get_bucket(vol_name)
            keys = []
            if key.endswith('*'):
                keys = b.list(prefix=key[0:-1])
            else:
                keys = [b.new_key(key)]
            returndata=[]

            for k in keys:
                value = k.get_contents_as_string()
                if value:
                    data = []
                    data += [('key' , k.name)]
                    data += [('md5sum' , md5.md5(value).hexdigest())]
                    data += [('length' , str(len(value)))]
                    data += [('begin' , str(value[:30]))]
                    data += [('end' , str(value[-30:]))]
                    returndata.append(tabulate(data, tablefmt=self.config.getTableFormat()))
                else:
                    returndata.append("--\nno data for key:{}\n".format(k.name))
            if len(returndata) == 0:
                returndata.append("--\nno data for key:{}\n".format(key))

            return '\n'.join(returndata)
        except Exception, e:
            log.exception(e)
            return 'get {} failed on volume: {}'.format(key, vol_name)

    #--------------------------------------------------------------------------------------
    @clidebugcmd
    @arg('prefix', help= "key prefix match", default='', nargs='?')
    def keys(self, vol_name, prefix):
        'get an object from the volume'
        try:
            if prefix.endswith('*'):
                prefix = prefix[0:-1]
            b = self.s3Api().get_bucket(vol_name)
            data = [[key.name.encode('ascii','ignore')] for key in b.list(prefix=prefix)]
            data.sort()
            return tabulate(data, tablefmt=self.config.getTableFormat(), headers=['name'])

        except Exception, e:
            log.exception(e)
            return 'get objects failed on volume: {}'.format(vol_name)

    #--------------------------------------------------------------------------------------
    @clicmd
    def deleteobject(self, vol_name, key):
        'delete an object from the volume'
        try:
            b = self.s3Api().get_bucket(vol_name)
            keys = []
            if key.endswith('*'):
                keys = b.list(prefix=key[0:-1])
            else:
                keys = [b.new_key(key)]
            for k in keys:
                k.delete()

        except Exception, e:
            log.exception(e)
            return 'get {} failed on volume: {}'.format(key, vol_name)

    #--------------------------------------------------------------------------------------
    @clidebugcmd
    @arg('volname', help='-volume name')
    def stats(self, volname):
        'display info about no. of objects/blobs'
        data = []
        volId = self.getVolumeId(volname)
        msg = FdspUtils.newSvcMsgByTypeId('StatVolumeMsg');
        msg.volume_id = volId
        dlt = self.config.getServiceApi().getDLT()
        errors = []
        callbacks={}
        for uuid in self.config.getServiceApi().getServiceIds('dm'):
            cb = WaitedCallback();
            try:
                self.config.getPlatform().sendAsyncSvcReq(uuid, msg, cb, None, dlt.version)
                callbacks[uuid]=cb
            except Exception,e :
                log.exception(e)
                print 'error on connecting to {}'.format(self.config.getServiceApi().getServiceName(uuid))

        for uuid, cb in callbacks.items():
            if not cb.wait(10):
                errors.append('volume stats failed:{} error:{}'.format(self.config.getServiceApi().getServiceName(uuid), cb.header.msg_code if cb.header!= None else "--"))
            else:
                #print cb.payload
                data.append((self.config.getServiceApi().getServiceName(uuid),
                             cb.payload.volumeStatus.blobCount,cb.payload.volumeStatus.objectCount,
                             cb.payload.volumeStatus.size, humanize.naturalsize(cb.payload.volumeStatus.size)))
        print ('\n{}\n stats for vol:{}\n{}'.format('-'*40, volId, '-'*40))
        print tabulate(data, tablefmt=self.config.getTableFormat(), headers=['dm','blobs','objects','size','size.human'])

        if len(errors) > 0:
            helpers.printHeader('errors detected ...')
            for e in errors:
                print e

    #--------------------------------------------------------------------------------------
    @clidebugcmd
    @arg('volname', help='-volume name')
    @arg('blobname', help='-blob name')
    @arg('--start', help='start offset', default=0)
    @arg('--end', help='end offset', default=-1)
    def blobinfo(self, volname, blobname, start=0, end=-1):
        'display objects/meta in a blob'
        data = []
        volId = self.getVolumeId(volname)
        dlt = self.config.getServiceApi().getDLT()

        volume = ServiceMap.omConfig().statVolume("", ServiceMap.omConfig().getVolumeName(volId))
        if volume.policy.volumeType == 0 and -1 == blobname.find(':'):
            blobname = 'user:{}'.format(blobname)
        errors = []
        for uuid in self.config.getServiceApi().getServiceIds('dm'):
            msg = FdspUtils.newSvcMsgByTypeId('QueryCatalogMsg');
            msg.volume_id = volId
            msg.blob_name = blobname
            msg.start_offset = start
            msg.end_offset = end
            cb = WaitedCallback();
            try:
                self.config.getPlatform().sendAsyncSvcReq(uuid, msg, cb, None, dlt.version)
            except Exception,e :
                log.exception(e)
                print 'error on connecting to {}'.format(self.config.getServiceApi().getServiceName(uuid))

            if not cb.wait(10):
                if cb.header == None:
                    break
                if cb.header.msg_code == 6:
                    errors.append('>>> blob [{}] not found @ {}'.format(blobname, self.config.getServiceApi().getServiceName(uuid)))
                else:
                    errors.append('>>> blobinfo request failed : {} : error={}'.format(self.config.getServiceApi().getServiceName(uuid), cb.header.msg_code  if cb.header!= None else "--"))
            else:
                data=[]
                objects=[]
                for meta in cb.payload.meta_list:
                    data.append((meta.key, meta.value))
                data.append(('size', cb.payload.byteCount))
                data.append(('num.objects', len(cb.payload.obj_list)))
                helpers.printHeader('info from {}'.format(self.config.getServiceApi().getServiceName(uuid)))
                print tabulate(data, tablefmt=self.config.getTableFormat(), headers=['key','value'])
                print
                count = 0
                for obj in cb.payload.obj_list:
                    count += 1
                    objects.append((count, binascii.b2a_hex(obj.data_obj_id.digest), obj.size, obj.offset))

                print tabulate(objects, tablefmt=self.config.getTableFormat(), headers=['no','objid','size','offset'])
        if len(errors) > 0:
            helpers.printHeader('errors detected ...')
            for e in errors:
                print e

    #--------------------------------------------------------------------------------------
    @clidebugcmd
    @arg('objid', help='-objectid')
    @arg('--service', help='services to query', default='sm')
    @arg('--size', help='size to dispplay', default=0)
    def getobject(self, objid, service='sm', size=0):
        'get objects from sm'
        data = []
        dlt = self.config.getServiceApi().getDLT()
        errors = []
        for uuid in self.config.getServiceApi().getServiceIds(service):
            msg = FdspUtils.newGetObjectMsg(1, binascii.a2b_hex(objid))
            cb = WaitedCallback();
            try:
                self.config.getPlatform().sendAsyncSvcReq(uuid, msg, cb, None, dlt.version)
            except Exception,e :
                log.exception(e)
                print 'error on connecting to {}'.format(self.config.getServiceApi().getServiceName(uuid))

            if not cb.wait(10):
                if cb.header.msg_code == 9:
                    errors.append('obj [{}] not found @ {}'.format(objid, self.config.getServiceApi().getServiceName(uuid)))
                else:
                    errors.append('get object failed : {} : error={}'.format(self.config.getServiceApi().getServiceName(uuid), cb.header.msg_code  if cb.header!= None else "--"))
                    print cb.header
            else:
                value = cb.payload.data_obj
                data = []
                data += [('objid' , objid)]
                data += [('sha1' , sha.sha(value).hexdigest())]
                data += [('md5sum' , md5.md5(value).hexdigest())]
                data += [('length' , str(len(value)))]
                data += [('begin' , str(value[:30]))]
                data += [('end' , str(value[-30:]))]
                if size > 0:
                    data += [('value', value[0:size])]

                helpers.printHeader('info from {}'.format(self.config.getServiceApi().getServiceName(uuid)))
                print tabulate(data, tablefmt=self.config.getTableFormat())

        if len(errors) > 0:
            helpers.printHeader('errors detected ...')
            for e in errors:
                print e
