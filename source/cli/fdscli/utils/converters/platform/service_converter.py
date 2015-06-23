import json
from model.platform.service import Service
from utils.converters.platform.service_status_converter import ServiceStatusConverter

class ServiceConverter():
    '''
    Created on Apr 28, 2015
    
    @author: nate
    '''
    
    @staticmethod
    def build_service_from_json( j_str ):
        
        if not isinstance(j_str, dict):
            j_str = json.loads(j_str)
        
        service = Service()
        
        service.type = j_str.pop("type", service.type)
        service.port = j_str.pop("port", service.port)
        service.id = j_str.pop("uid", service.id)
        service.name = j_str.pop("name", service.name)
        
        status = j_str.pop("status", service.status)
        service.status = ServiceStatusConverter.build_status_from_json(status)
        
        return service 
    
    @staticmethod
    def to_json( service ):
        d = dict()
        
        d["port"] = service.port
        d["status"] = json.loads(ServiceStatusConverter.to_json(service.status))
        d["uid"] = service.id
        d["name"] = service.name
        d["type"] = service.type
        
        result = json.dumps( d )
        
        return result
