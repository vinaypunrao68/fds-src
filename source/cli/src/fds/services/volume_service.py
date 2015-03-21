from rest_helper import RESTHelper
from fds.utils.volume_converter import VolumeConverter


class VolumeService():

    '''
    Created on Apr 3, 2015
    
    This class wraps all of the REST endpoints associated with volumes.  It "speaks" the volume model object
    and JSON and abstracts away all of the web protocols as a sort of volume SDK for FDS
    
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
    
    def listVolumes(self):
        
        '''
        Return the raw json list of volumes from the FDS REST call
        '''
        url = self.__get_url_preamble() + "/api/config/volumes"
        return self.__restHelper.get( self.__session, url )
    
    def createVolume(self, volume):
        '''
        Takes the passed in volume, converts it to JSON and uses the FDS REST
        endpoint to make the creation request
        '''
        
        url = self.__get_url_preamble() + "/api/config/volumes"
        data = VolumeConverter.to_json( volume )
        return self.__restHelper.post( self.__session, url, data )
