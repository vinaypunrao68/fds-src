import json
from model.platform.service_status import ServiceStatus

class ServiceStatusConverter(object):
    '''
    Created on Jun 1, 2015
    
    @author: nate
    '''

    @staticmethod
    def to_json(status):
        
        j_str = dict()
        
        j_str["state"] = status.state
        j_str["error_code"] = status.error_code
        j_str["description"] = status.description
        
        j_str = json.dumps(j_str)
        
        return j_str
    
    @staticmethod
    def build_status_from_json(j_str):
        
        if not isinstance(j_str, dict):
            j_str = json.loads(j_str)
            
        status = ServiceStatus()
        
        status.state = j_str.pop("state", status.state)
        status.description = j_str.pop("description", status.description)
        status.error_code = j_str.pop("error_code", status.error_code)
        
        return status
