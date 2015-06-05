from abstract_service import AbstractService
import json

from fds.utils.converters.platform.domain_converter import DomainConverter

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
        j_domains = self.rest_helper.get( self.session, url )
        
        domains = []
        
        for j_domain in j_domains:
            domain = DomainConverter.build_domain_from_json( j_domain )
            domains.append( domain )
            
        return domains
    
    def shutdown(self, domain):
        '''
        Shutdown the specified domain
        '''
        
        url = "{}{}{}".format( self.get_url_preamble(), "/local_domains/", domain.id )
        data = DomainConverter.to_json(domain)
        return self.rest_helper.put( self.session, url, data )
    
    def start(self, domain):
        '''
        start up the specified domain
        '''
        
        url = "{}{}{}".format( self.get_url_preamble(), "/local_domains/", domain.id )
        data = DomainConverter.to_json(domain)
        return self.rest_helper.put( self.session, url, data )
        
