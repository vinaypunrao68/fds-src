from rest_helper import RESTHelper
from fds.utils.volume_converter import VolumeConverter

'''
Created on Apr 3, 2015

This class wraps all of the REST endpoints associated with volumes.  It "speaks" the volume model object
and JSON and abstracts away all of the web protocols as a sort of volume SDK for FDS

@author: nate
'''
class VolumeService():
    
    def __init__(self, session):
        self.__session = session
        self.__restHelper = RESTHelper()
        
    '''
    Helper method to construct the base URI
    '''
    def __get_url_preamble(self):
        return "http://" + self.__session.get_hostname() + ":" + str( self.__session.get_port() )
    
    def listVolumes(self):
        
        url = self.__get_url_preamble() + "/api/config/volumes"
        return self.__restHelper.get( self.__session, url )
    
    def createVolume(self, volume):
        url = self.__get_url_preamble() + "/api/config/volumes"
        data = VolumeConverter.to_json( volume )
        return self.__restHelper.post( self.__session, url, data )
