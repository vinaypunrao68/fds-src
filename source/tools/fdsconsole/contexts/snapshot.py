from svchelper import *
from fdslib.pyfdsp.svc_types.ttypes import ResourceState
import humanize

class SnapshotContext(Context):
    def __init__(self, *args):
        Context.__init__(self, *args)

    #--------------------------------------------------------------------------------------
    @clidebugcmd
    @arg('vol-name', help= "-list snapshot for Volume name")
    @arg('--sortby', help='sort by name/time/delete', type=str)
    def list(self, vol_name,sortby='name'):
        'list snapshots for given volume'
        try:
            volume_id  = ServiceMap.omConfig().getVolumeId(vol_name);
            snapshot = ServiceMap.omConfig().listSnapshots(volume_id)
            if sortby == 'time':
                snapshot.sort(key=attrgetter('creationTimestamp'))
            elif sortby == 'delete':
                snapshot.sort(key=lambda s: s.creationTimestamp + s.retentionTimeSeconds)
            else:
                snapshot.sort(key=attrgetter('snapshotName'))
            return tabulate([( item.snapshotId, item.snapshotName, item.volumeId, item.snapshotPolicyId,
                               ResourceState._VALUES_TO_NAMES[item.state],
                               time.ctime(item.creationTimestamp), humanize.naturaldelta(item.retentionTimeSeconds), time.ctime(item.creationTimestamp+item.retentionTimeSeconds) ) for item in snapshot],
                            headers=['id', 'snapshot-name', 'vol',  'policy', 'state', 'create-date', 'retention', 'delete-date'], tablefmt=self.config.getTableFormat())
        except Exception, e:
            log.exception(e)
            return ' list snapshot polcies  snapshot polices for volume failed: {}'.format(vol_name)


    #--------------------------------------------------------------------------------------
    @clicmd
    @arg('vol-name', help= "volume name")
    @arg('snap-name', help= "name of the snapshot")
    @arg('retention', help= "retention time in seconds", nargs='?' , type=long, default=0)
    @arg('timeline-time', help= "timeline time  of parent volume", nargs='?' , type=long, default=0)
    def create(self, vol_name, snap_name, retention, timeline_time):
        'create a snaphot of the given volume'
        try:
            volume_id  = ServiceMap.omConfig().getVolumeId(vol_name);
            ServiceMap.omConfig().createSnapshot(volume_id, snap_name, retention, timeline_time)
            return 'ok'
        except Exception, e:
            log.exception(e)
            return ' create snapshot for volume failed: {}'.format(vol_name)


    #--------------------------------------------------------------------------------------
    @clicmd
    @arg('volume', help= "volume name/id")
    @arg('snapshot', help= "snapshot name/id")
    def delete(self, volume, snapshot):
        'create a snaphot of the given volume'
        try:
            if volume.isdigit():
                volume_id = int(volume)
            else:
                volume_id  = ServiceMap.omConfig().getVolumeId(volume);
            if snapshot.isdigit():
                snapshot_id = int(snapshot)
            else:
                snapshots = ServiceMap.omConfig().listSnapshots(volume_id)
                for snap in snapshots:
                    if snapshot == snap.snapshotName:
                        snapshot_id = snap.snapshotId
                    else:
                        snapshot_id = int(snapshot)
            ServiceMap.omConfig().deleteSnapshot(volume_id, snapshot_id)
            return 'ok'
        except Exception, e:
            log.exception(e)
            return ' create snapshot for volume failed: {}'.format(vol_name)


