import json
from model.volume.qos_policy import QosPolicy
class QosPolicyConverter(object):
    '''
    Created on Jun 1, 2015
    
    @author: nate
    '''
    
    @staticmethod
    def to_json( policy ):
        
        j_str = dict()
        
        j_str["priority"] = policy.priority
        j_str["iops_min"] = policy.iops_min
        j_str["iops_max"] = policy.iops_max
        
        j_str = json.dumps( j_str )
        
        return j_str
    
    @staticmethod
    def build_policy_from_json( j_str ):
        
        qos_policy = QosPolicy()
        
        qos_policy.priority = j_str.pop( "priority", qos_policy.priority )
        qos_policy.iops_max = j_str.pop( "iops_max", qos_policy.iops_max )
        qos_policy.iops_min = j_str.pop( "iops_min", qos_policy.iops_min )
        
        return qos_policy