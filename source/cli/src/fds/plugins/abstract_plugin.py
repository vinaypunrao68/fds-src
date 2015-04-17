'''
Created on Apr 13, 2015

@author: nate
'''

'''
This is an abstract class to define what methods need
to be overridden by a class that wishes to be a plugin
'''

class AbstractPlugin():

    def build_parser(self, parentParser, session): 
        raise NotImplementedError( "Required method for an FDS CLI plugin.")
    
    def detect_shortcut(self, args):
        raise NotImplementedError( "Required method for an FDS CLI plugin.")
    