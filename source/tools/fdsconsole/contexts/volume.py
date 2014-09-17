from  svchelper import *
from fdslib.pyfdsp.apis import ttypes
import pdb

class VolumeContext(Context):
    def __init__(self, *args):
        Context.__init__(self, *args)
        ServiceMap.init(self.config.getSystem('host'), self.config.getSystem('port'))

    def get_context_name(self):
        return "volume"

    #--------------------------------------------------------------------------------------
    @clicmd
    def list(self):
        try:
            volumes = ServiceMap.omConfig().listVolumes("")
            return volumes
        except Exception, e:
            log.exception(e)
            return 'unable to get volume list'

    #--------------------------------------------------------------------------------------
    @cliadmincmd
    @arg('vol-name', help= "-Volume name  of the clone")
    @arg('clone-name', help= "-name of  the  volume clone")
    @arg('policy-id', help= "-volume policy id" , default=0, type=int)
    def clone(self, vol_name, clone_name, policy_id):
        try:
            volume_id  = ServiceMap.omConfig().getVolumeId(vol_name)
            ServiceMap.omConfig().cloneVolume(volume_id, policy_name, clone_name )
            return 'Success'
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
            return 'Success'
        except Exception, e:
            log.exception(e)
            return 'restore clone failed: {}'.format(vol_name)


    #--------------------------------------------------------------------------------------
    @cliadmincmd
    @arg('vol-name', help='-volume name')
    @arg('--domain', help='-domain to add volume to')
    @arg('--max-obj-size', help='-maxiumum size (in bytes) of volume objects', type=int)
    @arg('--vol-type', help='-type of volume to create [object|block]', type=int)
    @arg('--blk-dev-size', help='-maximum size (in bytes) of block device', type=int)
    @arg('--tenant-id', help='-id of tenant to create volume under', type=int)
    def create(self, vol_name, domain='abc', max_obj_size=4096,
               vol_type=ttypes.VolumeType.OBJECT, blk_dev_size=21474836480, tenant_id=1):
        
        if vol_type == 'object':
            vol_type = ttypes.VolumeType.OBJECT
        elif vol_type == 'block':
            vol_type = ttypes.VolumeType.BLOCK
        elif vol_type not in (ttypes.VolumeType.OBJECT, ttypes.VolumeType.BLOCK):
            vol_type = ttypes.VolumeType.OBJECT

        vol_set = ttypes.VolumeSettings(max_obj_size, vol_type, blk_dev_size)
            
        try:
            ServiceMap.omConfig().createVolume(domain, vol_name, vol_set, tenant_id)
        except Exception, e:
            log.exception(e)
            return 'create volume failed: {}'.format(vol_name)

    @cliadmincmd
    @arg('--domain', help='-name of domain that volume resides in')
    @arg('vol-name', help='-volume name')
    def delete(self, vol_name, domain='abc'):
        try:
            ServiceMap.omConfig().deleteVolume(domain, vol_name)
        except Exception, e:
            log.exception(e)
            return 'delete volume failed: {}'.format(vol_name)
