import json
from model.volume.snapshot_policy import SnapshotPolicy
from utils.converters.volume.recurrence_rule_converter import RecurrenceRuleConverter

class SnapshotPolicyConverter():
    '''
    Created on Apr 23, 2015
    
    @author: nate
    '''
    @staticmethod
    def build_snapshot_policy_from_json( j_str ):
        
        snapshot_policy = SnapshotPolicy()
        
        if not isinstance( j_str, dict ):
            j_str = json.loads(j_str)
        
        snapshot_policy.id = j_str.pop( "uid", snapshot_policy.id )
        snapshot_policy.name = j_str.pop( "name", snapshot_policy.name )
        snapshot_policy.type = j_str.pop( "type", snapshot_policy.type )
        
        r_str = j_str.pop("retentionTime", snapshot_policy.retention_time_in_seconds)
        
        snapshot_policy.retention_time_in_seconds = r_str["seconds"]
        
        preset_id = j_str.pop("presetId", snapshot_policy.preset_id)
        
        if preset_id != "":
            snapshot_policy.preset_id = preset_id
        
        j_recur = j_str.pop( "recurrenceRule", snapshot_policy.recurrence_rule )
        
        if isinstance( j_recur, dict ):
            snapshot_policy.recurrence_rule = RecurrenceRuleConverter.build_rule_from_json( j_recur )
        
        return snapshot_policy

    @staticmethod
    def to_json( policy ):
        '''
        Convert a snapshot policy object to a JSON string
        '''
        
        d = dict()
        
        d["uid"] = policy.id
        d["name"] = policy.name
        d["type"] = policy.type
        
        if policy.preset_id is None:
            d["presetId"] = ""
        else:
            d["presetId"] = policy.preset_id
            
        retD = dict()
        retD["seconds"] = policy.retention_time_in_seconds
        retD["nanos"] = 0
            
        d["retentionTime"] = retD
        d["timelineTime"] = policy.timeline_time
        d["recurrenceRule"] = json.loads( RecurrenceRuleConverter.to_json( policy.recurrence_rule ) )
        
        result = json.dumps( d )
        
        return result
