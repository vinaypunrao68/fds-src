'''
Created on Apr 13, 2015

@author: nate
'''

'''
This is an abstract class to define what methods need
to be overridden by a class that wishes to be a plugin
'''

class AbstractPlugin( object ):

    def __init__(self, session):
        self.__session = session

    def build_parser(self, parentParser, session): 
        raise NotImplementedError( "Required method for an FDS CLI plugin.")
    
    def detect_shortcut(self, args):
        raise NotImplementedError( "Required method for an FDS CLI plugin.")
    
    @property
    def session(self):
        return self.__session