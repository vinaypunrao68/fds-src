from .abstract_service import AbstractService
from model.fds_error import FdsError

class EventService( AbstractService ):
    '''
    Created on Apr 23, 2015
    
    @author: nate
    '''

    def __init__(self, session):
        AbstractService.__init__(self, session)
        
    def query_system_events(self, event_filter):
        '''
        Get a list of system events that match your filter parameters
        '''
        
        url = "{}{}".format( self.get_url_preamble(), "/events")
        #TODO convert the filter to JSON
        data = "" 
        response = self.rest_helper().put( self.session, url, data )
        
        if isinstance(response, FdsError):
            return response
        
        return response        