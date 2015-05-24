import json
from ..model.service import Service

class ServiceConverter():
    '''
    Created on Apr 28, 2015
    
    @author: nate
    '''
    
    @staticmethod
    def build_service_from_json( jString ):
        
        service = Service()
        
        service.auto_name = jString.pop( "autoName", "UNKNOWN" )
        service.port = jString.pop( "port", 0 )
        service.type = jString.pop( "type", "FDS_PLATFORM" )
        service.id = jString.pop( "uuid", -1)
        service.status = jString.pop( "status", "UNKNOWN" )
        
        return service 
    
    @staticmethod
    def to_json( service ):
        d = dict()
        
        d["autoName"] = service.auto_name
        d["port"] = service.port
        d["status"] = service.status
        d["type"] = service.type
        d["uuid"] = service.id
        
        result = json.dumps( d )
        
        return result
