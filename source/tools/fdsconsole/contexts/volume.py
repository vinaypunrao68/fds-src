from svchelper import *
import  fdslib.pyfdsp.apis as apis
from apis import ttypes
from apis.ttypes import ApiException

import md5
import os
class VolumeContext(Context):
    def __init__(self, *args):
        Context.__init__(self, *args)
        self.s3api = self.config.s3rest
    #--------------------------------------------------------------------------------------
    @clicmd
    def list(self):
        'show the list of volumes in the system'
        try:
            volumes = ServiceMap.omConfig().listVolumes("")
            return tabulate([(item.name, item.tenantId, item.dateCreated,
                              'OBJECT' if item.policy.volumeType == 0 else 'BLOCK',
                              item.policy.maxObjectSizeInBytes, item.policy.blockDeviceSizeInBytes) for item in sorted(volumes, key=attrgetter('name'))  ],
                            headers=['Name', 'TenantId', 'Create Date','Type', 'Max-Obj-Size', 'Blk-Size'], tablefmt=self.config.getTableFormat())
            return volumes
        except ApiException, e:
            print e
        except Exception, e:
            log.exception(e)
        return 'unable to get volume list'

    #--------------------------------------------------------------------------------------
    @cliadmincmd
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
    @cliadmincmd
    @arg('vol-name', help= "-volume name  of the clone")
    @arg('clone-name', help= "-name of  the  volume clone for restore")
    def restore(self, vol_name, clone_name):
        try:
            volume_id  = ServiceMap.omConfig().getVolumeId(vol_name)
            ServiceMap.omConfig().restoreClone(volume_id, snapshotName)
            return
        except ApiException, e:
            print e
        except Exception, e:
            log.exception(e)
        return 'restore clone failed: {}'.format(vol_name)


    #--------------------------------------------------------------------------------------
    @cliadmincmd
    @arg('vol-name', help='-volume name')
    @arg('--domain', help='-domain to add volume to')
    @arg('--max-obj-size', help='-maxiumum size (in bytes) of volume objects', type=int)
    @arg('--vol-type', help='-type of volume to create', choices=['block','object'])
    @arg('--blk-dev-size', help='-maximum size (in bytes) of block device', type=int)
    @arg('--tenant-id', help='-id of tenant to create volume under', type=int)
    @arg('--commit-log-retention', help= " continuous commit log retention time in seconds", type=long)
    # @arg('--media-policy', help='-media policy for volume', choices=['ssd', 'hdd'])
    def create(self, vol_name, domain='abc', max_obj_size=2097152,
               vol_type='object', blk_dev_size=21474836480, tenant_id=1, commit_log_retention=86400 ):
               # media_policy='hdd'):
        
        if vol_type == 'object':
            vol_type = ttypes.VolumeType.OBJECT
        elif vol_type == 'block':
            vol_type = ttypes.VolumeType.BLOCK
            max_obj_size = 4096
        elif vol_type not in (ttypes.VolumeType.OBJECT, ttypes.VolumeType.BLOCK):
            vol_type = ttypes.VolumeType.OBJECT

        vol_set = ttypes.VolumeSettings(max_obj_size, vol_type, blk_dev_size, commit_log_retention)

        # if media_policy == 'hdd':
        #    media_policy = ttypes.FDSP_MediaPolicy.FDSP_MEDIA_POLICY_HDD
        # elif media_policy == 'ssd':
        #    media_policy = ttypes.FDSP_MediaPolicy.FDSP_MEDIA_POLICY_SSD
        # else:
        #    media_policy = ttypes.FDSP_MediaPolicy.FDSP_MEDIA_POLICY_HDD

        try:
            ServiceMap.omConfig().createVolume(domain, vol_name, vol_set, tenant_id)
            return
        except ApiException, e:
            print e
        except Exception, e:
            log.exception(e)

        return 'create volume failed: {}'.format(vol_name)

    #--------------------------------------------------------------------------------------
    @cliadmincmd
    @arg('--domain', help='-name of domain that volume resides in')
    @arg('vol-name', help='-volume name')
    def delete(self, vol_name, domain='abc'):
        'delete a volume'
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
            b = self.s3api.get_bucket(vol_name)
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
                print r.reason
        except Exception, e:
            log.exception(e)
            return 'put {} failed on volume: {}'.format(key, vol_name)


    #--------------------------------------------------------------------------------------
    @clidebugcmd
    def get(self, vol_name, key):
        'get an object from the volume'
        try:
            b = self.s3api.get_bucket(vol_name)
            k = b.new_key(key)
            value = k.get_contents_as_string()

            if value:
                data = []
                data += [('key' , key)]
                data += [('md5sum' , md5.md5(value).hexdigest())]
                data += [('length' , str(len(value)))]
                data += [('begin' , str(value[:30]))]
                data += [('end' , str(value[-30:]))]
                return tabulate(data, tablefmt=self.config.getTableFormat())
            else:
                print r.reason
        except Exception, e:
            log.exception(e)
            return 'get {} failed on volume: {}'.format(key, vol_name)

    #--------------------------------------------------------------------------------------
    @clidebugcmd
    def keys(self, vol_name):
        'get an object from the volume'
        try:
            b = self.s3api.get_bucket(vol_name)
            data = [[key.name.encode('ascii','ignore')] for key in b.list()]
            data.sort()
            return tabulate(data, tablefmt=self.config.getTableFormat(), headers=['name'])
            
        except Exception, e:
            log.exception(e)
            return 'get objects failed on volume: {}'.format(vol_name)

    #--------------------------------------------------------------------------------------

    @clidebugcmd
    def deleteobject(self, vol_name, key):
        'delete an object from the volume'
        try:
            b = self.s3api.get_bucket(vol_name)
            k = b.new_key(key)
            k.delete()            
        except Exception, e:
            log.exception(e)
            return 'get {} failed on volume: {}'.format(key, vol_name)
