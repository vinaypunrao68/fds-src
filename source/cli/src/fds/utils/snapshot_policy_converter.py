import json
from fds.model.snapshot_policy import SnapshotPolicy
from recurrence_rule_converter import RecurrenceRuleConverter

class SnapshotPolicyConverter():
    '''
    Created on Apr 23, 2015
    
    @author: nate
    '''
    @staticmethod
    def build_snapshot_policy_from_json( jsonString ):
        
        snapshot_policy = SnapshotPolicy()
        
        if isinstance( jsonString, dict ) == False:
            jsonString = json.loads(jsonString)
        
        snapshot_policy.name = jsonString.pop( "name", "" )
        snapshot_policy.retention = jsonString.pop( "retention", snapshot_policy.retention )
        snapshot_policy.timeline_time = jsonString.pop( "timelineTime", snapshot_policy.timeline_time )
        j_recur = jsonString.pop( "recurrenceRule", snapshot_policy.recurrence_rule )
        snapshot_policy.recurrence_rule = RecurrenceRuleConverter.build_rule_from_json( j_recur )
        snapshot_policy.id = jsonString.pop( "id", snapshot_policy.id )
        
        return snapshot_policy

    @staticmethod
    def to_json( policy ):
        '''
        Convert a snapshot policy object to a JSON string
        '''
        
        d = dict()
        
        d["name"] = policy.name
        d["retention"] = policy.retention
        d["timelineTime"] = policy.timeline_time
        d["recurrenceRule"] = json.loads( RecurrenceRuleConverter.to_json( policy.recurrence_rule ) )
        d["id"] = policy.id
        
        result = json.dumps( d )
        
        return result