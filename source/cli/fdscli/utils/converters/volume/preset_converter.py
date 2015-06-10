from model.volume.qos_preset import QosPreset
from model.volume.data_protection_policy_preset import DataProtectionPolicyPreset
from utils.converters.volume.snapshot_policy_converter import SnapshotPolicyConverter
import json

class PresetConverter(object):
    '''
    Created on May 6, 2015
    
    @author: nate
    '''

    @staticmethod
    def build_qos_preset_from_json( jsonString ):
        '''
        Take a JSON string and build a QoS preset from it
        '''
        
        qos = QosPreset()
        qos.id = jsonString.pop("id", qos.id)
        qos.name = jsonString.pop("name", "UNKNOWN")
        qos.iops_guarantee = jsonString.pop("iopsMin", qos.iops_min)
        qos.iops_limit = jsonString.pop("iopsMax", qos.iops_max)
        qos.priority = jsonString.pop("priority", qos.priority)
        
        return qos
    
    @staticmethod
    def build_timeline_from_json( jsonString ):
        '''
        Take a JSON string for a timeline preset and turn it into an object
        '''
        
        timeline = DataProtectionPolicyPreset()
        timeline.id = jsonString.pop("id", timeline.id)
        timeline.name = jsonString.pop("name", "UNKNOWN")
        
        cD = jsonString.pop( "commitLogRetention" )
        timeline.continuous_protection = cD["seconds"]
        
        #If I don't do this, the list is still populated with previous list... no idea why
        timeline.policies = list()
        
        policies = jsonString.pop("snapshotPolicies", [])
        
        for policy in policies:
            timeline.policies.append( SnapshotPolicyConverter.build_snapshot_policy_from_json( policy ))
            
        return timeline
    
    @staticmethod
    def qos_to_json( preset ):
        '''
        Turn a qos preset into JSON
        '''
        d = dict()
        
        d["id"] = preset.id
        d["name"] = preset.name
        d["priority"] = preset.priority
        d["iopsMin"] = preset.iops_min
        d["iopsMax"] = preset.iops_max
        
        j_str = json.dumps( d )
        
        return j_str
    
    @staticmethod
    def timeline_to_json(preset):
        '''
        Turn a timeline policy preset into a JSON string
        '''
