from .abstract_service import AbstractService

from utils.converters.platform.domain_converter import DomainConverter
from model.fds_error import FdsError

class LocalDomainService( AbstractService ):
    '''
    Created on Apr 28, 2015
    
    @author: nate
    '''

    def __init__(self, session):
        AbstractService.__init__(self, session)

    def find_domain_by_id(self, domain_id):
        
        domains = self.get_local_domains()
        
        for domain in domains:
            if int(domain.id) == int(domain_id):
                return domain
            
        return None

    def get_local_domains(self):
        '''
        Retrieve a list of local domains
        '''
        
        url = "{}{}".format(self.get_url_preamble(), "/local_domains")
        j_domains = self.rest_helper.get( self.session, url )
        
        if isinstance(j_domains, FdsError):
            return j_domains
        
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
        domain.state = "DOWN"
        data = DomainConverter.to_json(domain)
        response = self.rest_helper.put( self.session, url, data )
        
        if isinstance(response, FdsError):
            return response
        
        return response        
    
    def start(self, domain):
        '''
        start up the specified domain
        '''
        
        url = "{}{}{}".format( self.get_url_preamble(), "/local_domains/", domain.id )
        domain.state = "UP"
        data = DomainConverter.to_json(domain)
        response = self.rest_helper.put( self.session, url, data )
        
        if isinstance(response, FdsError):
            return response
        
        return response        
        
