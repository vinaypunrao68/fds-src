from svchelper import *

class SnapshotContext(Context):
    def __init__(self, *args):
        Context.__init__(self, *args)
        ServiceMap.init(self.config.getSystem('host'), self.config.getSystem('port'))

    def get_context_name(self):
        return "snapshot"

    #--------------------------------------------------------------------------------------
    @clicmd    
    @arg('vol-name', help= "-list snapshot for Volume name")
    def list(vol_name):
        try:
            volume_id  = ServiceMap.omConfig().getVolumeId(vol_name);
            snapshot = ServiceMap.omConfig().listSnapshots(volume_id)
            return tabulate([(item.snapshotName, item.volumeId, item.snapshotId, item.snapshotPolicyId, time.ctime((item.creationTimestamp)/1000)) for item in snapshot],
                            headers=['Snapshot-name', 'volume-Id', 'Snapshot-Id', 'policy-Id', 'Creation-Time'])
        except Exception, e:
            log.exception(e)
            return ' list snapshot polcies  snapshot polices for volume failed: {}'.format(vol_name)


