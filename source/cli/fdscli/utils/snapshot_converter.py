from ..model.snapshot import Snapshot
import json

class SnapshotConverter():
    '''
    Created on Apr 20, 2015

    @author: nate
    '''
    @staticmethod
    def build_snapshot_from_json( jsonData ):
        
        snapshot = Snapshot()
        
        snapshot.id = jsonData.pop( "id", snapshot.id )
        snapshot.name = jsonData.pop( "name", snapshot.name)
        snapshot.retention = jsonData.pop( "retention", snapshot.retention)
        snapshot.timeline_time = jsonData.pop( "timelineTime", snapshot.timeline_time)
        snapshot.volume_id = jsonData.pop( "volumeId", snapshot.volume_id)
        snapshot.created = jsonData.pop( "created", snapshot.created)
        
        return snapshot
    
    @staticmethod
    def to_json( snapshot ):
        
        d = dict()
        
        d["name"] = snapshot.name
        d["id"] = snapshot.id
        d["retention"] = snapshot.retention
        d["timelineTime"] = snapshot.timeline_time
        d["volumeId"] = snapshot.volume_id
        d["created"] = snapshot.created
        
        result = json.dumps( d )
        
        return result
        
