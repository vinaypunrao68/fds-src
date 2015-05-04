import json
from fds.model.domain import Domain

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
        
        domain.name = jsonString.pop( "name", domain.name )
        domain.id = jsonString.pop( "id", domain.id )
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