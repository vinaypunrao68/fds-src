import json
from fds.model.volume.snapshot_policy import SnapshotPolicy
from fds.utils.converters.volume.recurrence_rule_converter import RecurrenceRuleConverter
from fds.utils.converters.fds_id_converter import FdsIdConverter
from fds.utils.converters.common.duration_converter import DurationConverter

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
        
        snapshot_policy.id = FdsIdConverter.build_id_from_json(j_str.pop("id", snapshot_policy.id))
        
        r_str = j_str.pop("retention_time_in_seconds", snapshot_policy.retention_time_in_seconds)
        
        snapshot_policy.retention_time_in_seconds = r_str
        snapshot_policy.timeline_time = j_str.pop( "timeline_time", snapshot_policy.timeline_time )
        
        preset_id = j_str.pop("preset_id", snapshot_policy.preset_id)
        
        if preset_id != "":
            snapshot_policy.preset_id = FdsIdConverter.build_id_from_json(preset_id)
        
        j_recur = j_str.pop( "recurrence_rule", snapshot_policy.recurrence_rule )
        
        if isinstance( j_recur, dict ):
            snapshot_policy.recurrence_rule = RecurrenceRuleConverter.build_rule_from_json( j_recur )
        
        return snapshot_policy

    @staticmethod
    def to_json( policy ):
        '''
        Convert a snapshot policy object to a JSON string
        '''
        
        d = dict()
        
        d["id"] = json.loads(FdsIdConverter.to_json(policy.id))
        
        if policy.preset_id is None:
            d["preset_id"] = ""
        else:
            d["preset_id"] = json.loads(FdsIdConverter.to_json(policy.preset_id))
            
        d["retention_time_in_seconds"] = policy.retention_time_in_seconds
        d["timeline_time"] = policy.timeline_time
        d["recurrence_rule"] = json.loads( RecurrenceRuleConverter.to_json( policy.recurrence_rule ) )
        
        result = json.dumps( d )
        
        return result
