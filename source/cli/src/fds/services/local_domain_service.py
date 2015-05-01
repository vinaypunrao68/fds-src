from abstract_service import AbstractService
import json

class LocalDomainService( AbstractService ):
    '''
    Created on Apr 28, 2015
    
    @author: nate
    '''

    def __init__(self, session):
        AbstractService.__init__(self, session)

    def get_local_domains(self):
        '''
        Retrieve a list of local domains
        '''
        
        url = "{}{}".format(self.get_url_preamble(), "/local_domains")
        return self.rest_helper.get( self.session, url )
    
    def shutdown(self, domain_name):
        '''
        Shutdown the specified domain
        '''
        
        url = "{}{}{}".format( self.get_url_preamble(), "/local_domains/", domain_name )
        data = dict();
        data["action"] = "shutdown"
        
        data = json.dumps( data )
        
        return self.rest_helper.put( self.session, url, data )
        