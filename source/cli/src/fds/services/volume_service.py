'''
Created on Apr 3, 2015

@author: nate
'''
from rest_helper import RESTHelper
class VolumeService():
    
    def __init__(self, session):
        self.__session = session
        self.__restHelper = RESTHelper()
    
    def listVolumes(self):
        
        url = 'http://'+ self.__session.get_hostname() + ":" + str( self.__session.get_port() ) + "/api/config/volumes"
        return self.__restHelper.get( self.__session, url )
