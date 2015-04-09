'''
Created on Apr 3, 2015

@author: nate
'''
class VolumeService():
    
    def __init__(self, RESTHelper):
        self.__restHelper = RESTHelper
    
    def listVolumes(self):
        
        url = 'http://localhost:7777/api/config/volumes'
        return self.__restHelper.get( url )
