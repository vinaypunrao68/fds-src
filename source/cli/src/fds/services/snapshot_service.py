from rest_helper import RESTHelper

class SnapshotService():

    '''
    Created on Apr 22, 2015
    
    This class wraps all of the REST endpoints associated with snapshots.  It "speaks" the snapshot model object
    and JSON and abstracts away all of the web protocols as a sort of snapshot SDK for FDS
    
    @author: nate
    '''
    
    def __init__(self, session):
        self.__session = session
        self.__restHelper = RESTHelper()
        
    def __get_url_preamble(self):
        '''
        Helper method to construct the base URI
        '''        
        return "http://" + self.__session.get_hostname() + ":" + str( self.__session.get_port() )
    
    def get_snapshot_by_id(self, an_id):
        '''
        retrieve a specific snapshot using its ID
        '''
        
        url = self.__get_url_preamble() + "/api/config/snapshots/" + an_id
        return self.__restHelper.get( self.__session, url )        
