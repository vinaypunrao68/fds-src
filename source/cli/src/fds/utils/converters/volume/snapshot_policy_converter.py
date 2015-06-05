import json
from fds.model.volume.snapshot_policy import SnapshotPolicy
from fds.utils.converters.volume.recurrence_rule_converter import RecurrenceRuleConverter
from fds.utils.converters.fds_id_converter import FdsIdConverter

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
        
        r_str = j_str.pop("retentionTimeInSeconds", snapshot_policy.retention_time_in_seconds)
        
        snapshot_policy.retention_time_in_seconds = r_str
        snapshot_policy.timeline_time = j_str.pop( "timelineTime", snapshot_policy.timeline_time )
        
        preset_id = j_str.pop("presetId", snapshot_policy.preset_id)
        
        if preset_id != "":
            snapshot_policy.preset_id = FdsIdConverter.build_id_from_json(preset_id)
        
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
        
        if policy.preset_id is None:
            d["presetId"] = ""
        else:
            d["presetId"] = json.loads(FdsIdConverter.to_json(policy.preset_id))
            
        d["retentionTimeInSeconds"] = policy.retention_time_in_seconds
        d["timelineTime"] = policy.timeline_time
        d["recurrenceRule"] = json.loads( RecurrenceRuleConverter.to_json( policy.recurrence_rule ) )
        
        result = json.dumps( d )
        
        return result
