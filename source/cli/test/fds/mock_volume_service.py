'''
Created on Apr 9, 2015

@author: nate
'''
from mock import Mock

class MockVolumeService(Mock):
    
    __volumes = []
    
    def createVolume(self, volume):
        
        volume.set_id( len( self.__volumes ) )
        self.__volumes.append( volume )
        return volume
    
    def listVolumes(self):
        return self.__volumes