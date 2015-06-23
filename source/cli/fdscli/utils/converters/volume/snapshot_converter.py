from model.volume.snapshot import Snapshot
import json

class SnapshotConverter():
    '''
    Created on Apr 20, 2015

    @author: nate
    '''
    @staticmethod
    def build_snapshot_from_json( jsonData ):
        
        snapshot = Snapshot()
        
        snapshot.id = jsonData.pop( "uid", snapshot.id )
        snapshot.name = jsonData.pop( "name", snapshot.name)
        
        retD = jsonData.pop("retention", snapshot.retention)
        snapshot.retention = int(retD["seconds"])
        
        snapshot.volume_id = jsonData.pop( "volumeId", snapshot.volume_id)
        
        createD = jsonData.pop( "creationTime", snapshot.created )
        snapshot.created = int(createD["seconds"])
        
        return snapshot
    
    @staticmethod
    def to_json( snapshot ):
        
        d = dict()
        
        d["name"] = snapshot.name
        d["id"] = snapshot.id
        
        retD = dict()
        retD["seconds"] = int(snapshot.retention)
        retD["nanos"] = 0
        d["retention"] = retD
        
        d["volumeId"] = snapshot.volume_id
        
        createD = dict()
        createD["seconds"] = int(snapshot.created)
        createD["nanos"] = 0
        d["creationTime"] = createD
        
        result = json.dumps( d )
        
        return result
        
