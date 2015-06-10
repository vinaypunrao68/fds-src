import json
from model.platform.domain import Domain

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
        
        domain.id = j_str.pop("uid", domain.id)
        domain.name = j_str.pop("name", domain.name)
        domain.site = j_str.pop( "site", domain.site)
        domain.state = j_str.pop("state", domain.state)
        
        return domain
    
    @staticmethod
    def to_json( domain ):
        
        j_d = dict()
        
        j_d["uid"] = domain.id
        j_d["name"] = domain.name
        j_d["site"] = domain.site
        j_d["state"] = domain.state
        
        domain_str = json.dumps( j_d )
        
        return domain_str
