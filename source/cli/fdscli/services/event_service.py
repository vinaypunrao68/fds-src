from abstract_service import AbstractService

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
        
        url = "{}{}".format( self.get_url_preamble(), "/api/config/events")
        #TODO convert the filter to JSON
        data = "" 
        return self.rest_helper().put( self.session, url, data )