import abstract_plugin

'''
Created on Apr 13, 2015

A plugin to gather the system health data and set the parsing up 
for that operation

@author: nate
'''
class SystemHealthPlugin( abstract_plugin.AbstractPlugin):
    
    def __init__(self, session):
        abstract_plugin.AbstractPlugin.__init__(self, session)    
    
    '''
    @see: AbstractPlugin
    '''
    def build_parser(self, parentParser, session): 
        
        self.__parser = parentParser.add_parser( "system_health", help="Query the system health" )
        self.__subparser = self.__parser.add_subparsers( help="The sub-commands that are available")
    
    '''
    @see: AbstractPlugin
    '''    
    def detect_shortcut(self, args):
        
        return None