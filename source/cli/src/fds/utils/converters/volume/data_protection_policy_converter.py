from fds.utils.converters.fds_id_converter import FdsIdConverter
from fds.utils.converters.volume.snapshot_policy_converter import SnapshotPolicyConverter
import json
from fds.model.volume.data_protection_policy import DataProtectionPolicy
from fds.utils.converters.common.duration_converter import DurationConverter

class DataProtectionPolicyConverter(object):
    '''
    Created on Jun 1, 2015
    
    @author: nate
    '''

    @staticmethod
    def to_json(policy):
        
        j_str = dict()
        
        j_str["commit_log_retention"] = json.loads(DurationConverter.to_json(policy.commit_log_retention))
        
        if policy.preset_id is None:
            j_str["preset_id"] = ""
        else:
            j_str["preset_id"] = json.loads(FdsIdConverter.to_json(policy.preset_id))
        
        j_policies = []
        
        for s_policy in policy.snapshot_policies:
            j_policy = json.loads(SnapshotPolicyConverter.to_json(s_policy))
            j_policies.append( j_policy )
            
        j_str["snapshot_policies"] = j_policies
        
        j_str = json.dumps(j_str)
        
        return j_str
    
    @staticmethod
    def build_policy_from_json(j_str):
        
        policy = DataProtectionPolicy()
        
        preset_id = j_str.pop("preset_id", policy.preset_id)
        
        if preset_id == "":
            policy.preset_id = None
        else:
            policy.preset_id = preset_id
            
        policy.commit_log_retention = DurationConverter.build_duration_from_json(j_str.pop("commit_log_retention", policy.commit_log_retention))
        
        j_policies = j_str.pop("snapshot_policies", policy.snapshot_policies)
        
        for j_policy in j_policies:
            s_policy = SnapshotPolicyConverter.build_snapshot_policy_from_json(j_policy)
            policy.snapshot_policies.append(s_policy)
            
        return policy
            