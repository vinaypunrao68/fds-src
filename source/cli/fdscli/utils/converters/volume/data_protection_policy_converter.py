from utils.converters.volume.snapshot_policy_converter import SnapshotPolicyConverter
import json
from model.volume.data_protection_policy import DataProtectionPolicy

class DataProtectionPolicyConverter(object):
    '''
    Created on Jun 1, 2015
    
    @author: nate
    '''

    @staticmethod
    def to_json(policy):
        
        j_str = dict()
        
        commitD = dict()
        commitD["seconds"] = policy.commit_log_retention
        commitD["nanos"] = 0
        j_str["commitLogRetention"] = commitD
        
        if policy.preset_id is None:
            j_str["presetId"] = ""
        else:
            j_str["presetId"] = policy.preset_id
        
        j_policies = []
        
        for s_policy in policy.snapshot_policies:
            j_policy = json.loads(SnapshotPolicyConverter.to_json(s_policy))
            j_policies.append( j_policy )
            
        j_str["snapshotPolicies"] = j_policies
        
        j_str = json.dumps(j_str)
        
        return j_str
    
    @staticmethod
    def build_policy_from_json(j_str):
        
        policy = DataProtectionPolicy()
        
        preset_id = j_str.pop("presetId", policy.preset_id)
        
        if preset_id == "":
            policy.preset_id = None
        else:
            policy.preset_id = preset_id
            
        commitD = j_str.pop( "commitLogRetention", policy.commit_log_retention )
        policy.commit_log_retention = commitD["seconds"]
        
        j_policies = j_str.pop("snapshotPolicies", policy.snapshot_policies)
        
        for j_policy in j_policies:
            s_policy = SnapshotPolicyConverter.build_snapshot_policy_from_json(j_policy)
            policy.snapshot_policies.append(s_policy)
            
        return policy
            
