from model.health.system_health import SystemHealth
from utils.converters.health.health_record_converter import HealthRecordConverter
import json

class SystemHealthConverter(object):
    '''
    Created on Jun 29, 2015
    
    @author: nate
    '''

    @staticmethod
    def to_json(system_health):
        
#         if not isinstance(system_health, SystemHealth):
#             raise TypeError()
        
        j_str = dict()
        
        j_str["overall"] = system_health.overall_health
        
        j_healths = []
        
        for health_record in system_health.health_records:
            j_health = HealthRecordConverter.to_json(health_record)
            j_health = json.loads(j_health)
            j_healths.append(j_health)
            
        j_str["status"] = j_healths
        
        j_str = json.dumps(j_str)
        
        return j_str
    
    @staticmethod
    def build_system_health_from_json(j_str):
        
        system_health = SystemHealth()
        
        if not isinstance(j_str, dict):
            j_str = json.loads(j_str)
        
        system_health.overall_health = j_str.pop("overall", system_health.overall_health)
        
        j_healths = j_str.pop("status")
        
        for j_health in j_healths:
            health = HealthRecordConverter.build_health_record_from_json(j_health)
            system_health.health_records.append( health )
            
        return system_health
        