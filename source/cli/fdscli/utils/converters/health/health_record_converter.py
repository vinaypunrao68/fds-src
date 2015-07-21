from model.health.health_record import HealthRecord
import json

class HealthRecordConverter(object):
    '''
    Created on Jun 29, 2015
    
    @author: nate
    '''

    @staticmethod
    def to_json(health_record):
        
#         if not isinstance(health_record, HealthRecord):
#             raise TypeError()
        
        j_str = dict()
        
        j_str["state"] = health_record.state
        j_str["category"] = health_record.category
        j_str["message"] = health_record.message
        
        j_str = json.dumps(j_str)
        
        return j_str
    
    @staticmethod
    def build_health_record_from_json(j_str):
        
        health_record = HealthRecord()
        
        if not isinstance(j_str, dict):
            j_str = json.loads(j_str)
        
        health_record.state = j_str.pop("state", health_record.state)
        health_record.category = j_str.pop("category", health_record.category)
        health_record.message = j_str.pop("message", health_record.message)
        
        return health_record