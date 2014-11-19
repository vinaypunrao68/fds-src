from svchelper import *
from fdslib.pyfdsp.common.ttypes import ResourceState


class SnapshotContext(Context):
    def __init__(self, *args):
        Context.__init__(self, *args)

    #--------------------------------------------------------------------------------------
    @clicmd    
    @arg('vol-name', help= "-list snapshot for Volume name")
    @arg('--sortby', help='sort by name*/time', type=str)
    def list(self, vol_name,sortby='name'):
        try:
            volume_id  = ServiceMap.omConfig().getVolumeId(vol_name);
            snapshot = ServiceMap.omConfig().listSnapshots(volume_id)
            if sortby == 'time':
                snapshot.sort(key=attrgetter('creationTimestamp'))
            else:
                snapshot.sort(key=attrgetter('snapshotName'))
            return tabulate([( item.snapshotId, item.snapshotName, item.volumeId, item.snapshotPolicyId,
                               ResourceState._VALUES_TO_NAMES[item.state],
                               time.ctime((item.creationTimestamp)/1000)) for item in snapshot],
                            headers=['Id', 'Snapshot-name', 'volume-Id',  'policy-Id', 'State', 'Creation-Time'], tablefmt=self.config.getTableFormat())
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


