import json
from model.platform.service import Service
from utils.converters.platform.service_status_converter import ServiceStatusConverter
from utils.converters.fds_id_converter import FdsIdConverter

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
        service.id = FdsIdConverter.build_id_from_json(j_str.pop("id"))
        service.status = ServiceStatusConverter.build_status_from_json(j_str.pop("status"))
        
        return service 
    
    @staticmethod
    def to_json( service ):
        d = dict()
        
        d["port"] = service.port
        d["status"] = json.loads(ServiceStatusConverter.to_json(service.status))
        d["id"] = json.loads(FdsIdConverter.to_json(service.id))
        d["type"] = service.type
        
        result = json.dumps( d )
        
        return result
