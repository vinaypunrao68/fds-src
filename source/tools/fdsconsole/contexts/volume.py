from svchelper import *
from svc_types.ttypes import *
from common.ttypes import *
from platformservice import *
from restendpoint import *

import md5
import os
import FdspUtils

class VolumeContext(Context):
    def __init__(self, *args):
        Context.__init__(self, *args)
        self.__restApi = None

    def restApi(self):
        if self.__restApi == None:
            self.__restApi = VolumeEndpoint(self.config.getRestApi())
        return self.__restApi

    def s3Api(self):
        return self.config.getS3Api()

    def getVolumeId(self, volume):
        client = self.config.getPlatform();
        volId = client.svcMap.omConfig().getVolumeId(volume)
        return int(volId)

    #--------------------------------------------------------------------------------------
    @clicmd
    def list(self):
        'show the list of volumes in the system'
        try:
            volumes = ServiceMap.omConfig().listVolumes("")
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
    @arg('vol-name', help='-volume name')
    @arg('--minimum', help='-qos minimum guarantee', type=int)
    @arg('--maximum', help='-qos maximum', type=int)
    @arg('--priority', help='-qos priority', type=int)
    def modify(self, vol_name, domain='abc', max_obj_size=2097152, tenant_id=1,
               minimum=0, maximum=0, priority=10):
        'modify an existing volume'
        try:
            vols = self.restApi().listVolumes()
            vol_id = None
            mediaPolicy = None
            commit_log_retention = None
            for vol in vols:
                if vol['name'] == vol_name:
                    vol_id = vol['id']
                    mediaPolicy = vol['mediaPolicy']
                    commit_log_retention= vol['commit_log_retention']
            assert not vol_id is None
            assert not mediaPolicy is None
            assert not commit_log_retention is None
            res = self.restApi().setVolumeParams(vol_id, minimum, priority, maximum, mediaPolicy, commit_log_retention)
            return
        except ApiException, e:
            log.exception(e)
        except Exception, e:
            log.exception(e)

        return 'modify volume failed: {}'.format(vol_name)


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

            dmUuids = dmClient.svcMap.svcUuids('dm')
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
            listcb = WaitedCallback();
            if patternSemantics == 'PCRE':
                getblobmsg.patternSemantics = 0
            elif patternSemantics == 'PREFIX':
                getblobmsg.patternSemantics = 1
            elif patternSemantics == 'PREFIX_AND_DELIMITER':
                getblobmsg.patternSemantics = 2
            getblobmsg.delimiter = delimiter
            dmClient.sendAsyncSvcReq(dmUuids[0], getblobmsg, listcb)

            if not listcb.wait():
                print 'async listblob request failed'

            #import pdb; pdb.set_trace()
            blobs = listcb.payload.blob_info_list;
            # blobs.sort(key=attrgetter('blob_name'))
            return tabulate([(x.blob_name, x.blob_size) for x in blobs],headers=
                 ['blobname', 'blobsize'], tablefmt=self.config.getTableFormat())

        except Exception, e:
            log.exception(e)
            return 'unable to get volume meta list'

    #--------------------------------------------------------------------------------------
    @clidebugcmd
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
    @clidebugcmd
    @arg('cnt', help= "count of objs to get", type=long, default=100)
    def bulkget(self, vol_name, cnt):
        '''
        Does bulk gets.
        TODO: Make it do random gets as well
        '''
        b = self.s3Api().get_bucket(vol_name)
        i = 0
        while i < cnt:
            for key in b.list():
                print "Getting object#: {}".format(i)
                print self.get(vol_name, key.name.encode('ascii','ignore'))
                i = i + 1
                if i >= cnt:
                    break
        return 'Done!'

    #--------------------------------------------------------------------------------------
    @clidebugcmd
    @arg('cnt', help= "count of objs to put", type=long, default=100)
    @arg('seed', help= "count of objs to put", default='seed')
    def bulkput(self, vol_name, cnt, seed):
        '''
        Does bulk put
        TODO: Make it do random put as well
        '''
        for i in xrange(0, cnt):
            k = "key_{}_{}".format(seed, i)
            v = "value_{}_{}".format(seed, i)
            print "Putting object#: {}".format(i)
            print self.put(vol_name, k, v)
        return 'Done!'
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
        svc = self.config.getPlatform();
        for uuid in self.config.getServiceApi().getServiceIds('dm'):
            volId = self.getVolumeId(volname)
            print volId
            getblobmeta = FdspUtils.newGetVolumeMetaDataMsg(volId);
            cb = WaitedCallback();
            svc.sendAsyncSvcReq(uuid, getblobmeta, cb)

            if not cb.wait(10):
                print 'async volume meta request failed : {}'.format(cb.header)
            else:
                print cb.payload
                data += [("numblobs",cb.payload.volume_meta_data.blobCount)]
                data += [("size",cb.payload.volume_meta_data.size)]
                data += [("numobjects",cb.payload.volume_meta_data.objectCount)]
                print data
