from services.rest_helper import RESTHelper

class AbstractService():
    '''
    Created on Apr 23, 2015
    
    Base class that adds some helpers to extended services
    
    @author: nate
    '''
    
    def __init__(self, session):
        self.__session = session
        self.__rest_helper = RESTHelper()
        
    def get_url_preamble(self):
        '''
        Helper method to construct the base URI
        '''        
        return "http://{}:{}{}".format( self.session.get_hostname(), self.session.get_port(), "/fds/config/v08" ) 
        
    @property
    def session(self):
        return self.__session
    
    @property
    def rest_helper(self):
        return self.__rest_helper
