'''
Created on Apr 9, 2015

@author: nate
'''
from mock import Mock

class MockFdsAuth(Mock):
    
    def login(self):
        return "thisisavalidmocktoken"