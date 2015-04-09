'''
Created on Apr 9, 2015

@author: nate
'''
from mock import Mock

class MockVolumeService(Mock):
    
    def listVolumes(self):
        volumes = [{'id':1},{'id':2}]
        return volumes