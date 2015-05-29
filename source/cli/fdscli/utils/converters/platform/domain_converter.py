import json
from model.platform.domain import Domain
from model.fds_id import FdsId
from utils.converters.fds_id_converter import FdsIdConverter

class DomainConverter():
    '''
    Created on Apr 28, 2015
    
    @author: nate
    '''
    
    @staticmethod
    def build_domain_from_json( jsonString ):
        
        domain = Domain()
        
        if not isinstance( jsonString, dict ):
            jsonString = json.loads( jsonString )
        
        domain.id = FdsId()
        
        id_dict = jsonString.pop( "id", dict() )
        
        domain.id = FdsIdConverter.build_id_from_json( id_dict )
        domain.site = jsonString.pop( "site", domain.site )
        
        return domain
    
    @staticmethod
    def to_json( domain ):
        
        j_d = dict()
        
        j_d["name"] = domain.name
        j_d["id"] = domain.id
        j_d["site"] = domain.site
        
        domain_str = json.dumps( j_d )
        
        return domain_str
