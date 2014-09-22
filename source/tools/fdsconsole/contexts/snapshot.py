from svchelper import *

class SnapshotContext(Context):
    def __init__(self, *args):
        Context.__init__(self, *args)

    #--------------------------------------------------------------------------------------
    @clicmd    
    @arg('vol-name', help= "-list snapshot for Volume name")
    def list(self, vol_name):
        try:
            volume_id  = ServiceMap.omConfig().getVolumeId(vol_name);
            snapshot = ServiceMap.omConfig().listSnapshots(volume_id)
            return tabulate([(item.snapshotName, item.volumeId, item.snapshotId, item.snapshotPolicyId, time.ctime((item.creationTimestamp)/1000)) for item in snapshot],
                            headers=['Snapshot-name', 'volume-Id', 'Snapshot-Id', 'policy-Id', 'Creation-Time'])
        except Exception, e:
            log.exception(e)
            return ' list snapshot polcies  snapshot polices for volume failed: {}'.format(vol_name)


    #--------------------------------------------------------------------------------------
    @clicmd
    @arg('vol-name', help= "volume name")
    @arg('snap-name', help= "name of the snapshot")
    @arg('retention', help= "retention time in seconds", nargs='?' , type=long, default=0)
    def create(self, vol_name, snap_name, retention):
        'create a snaphot of the given volume'
        try:
            volume_id  = ServiceMap.omConfig().getVolumeId(vol_name);
            snapshot = ServiceMap.omConfig().createSnapshot(volume_id, snap_name, retention)
            return 'ok'
        except Exception, e:
            log.exception(e)
            return ' create snapshot for volume failed: {}'.format(vol_name)


