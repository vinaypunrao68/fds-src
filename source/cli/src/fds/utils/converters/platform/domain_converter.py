import json
from model.platform.domain import Domain
from utils.converters.fds_id_converter import FdsIdConverter

class DomainConverter():
    '''
    Created on Apr 28, 2015
    
    @author: nate
    '''
    
    @staticmethod
    def build_domain_from_json( j_str ):
        
        domain = Domain()
        
        if not isinstance( j_str, dict ):
            j_str = json.loads( j_str )
        
        domain.id = FdsIdConverter.build_id_from_json( j_str.pop("id") )
        domain.site = j_str.pop( "site", j_str.pop("site") )
        
        return domain
    
    @staticmethod
    def to_json( domain ):
        
        j_d = dict()
        
        j_d["id"] = json.loads(FdsIdConverter.to_json(domain.id))
        j_d["site"] = domain.site
        
        domain_str = json.dumps( j_d )
        
        return domain_str
