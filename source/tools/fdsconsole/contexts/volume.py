from  svchelper import *

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
    @arg('policy-name', help= "-clone policy name")
    def clone(self, vol_name, clone_name, policy_name):
        try:
            volume_id  = ServiceMap.omConfig().getVolumeId(vol_name);
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
            volume_id  = ServiceMap.omConfig().getVolumeId(vol_name);
            ServiceMap.omConfig().restoreClone(volume_id, snapshotName)
            return 'Success'
        except Exception, e:
            log.exception(e)
            return 'restore clone failed: {}'.format(vol_name)

