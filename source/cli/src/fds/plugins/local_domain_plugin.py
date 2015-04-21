import abstract_plugin

'''
Created on Apr 13, 2015

A plugin to configure all the parsing for local domain operations and 
to use those arguments to make the appropriate local domain REST calls

@author: nate
'''
class LocalDomainPlugin( abstract_plugin.AbstractPlugin):
    
    def __init__(self, session):
        abstract_plugin.AbstractPlugin.__init__(self, session)    
    
    '''
    @see: AbstractPlugin
    '''
    def build_parser(self, parentParser, session): 
        
        self.__parser = parentParser.add_parser( "local_domain", help="Manage and interact with local domains" )
        self.__subparser = self.__parser.add_subparsers( help="The sub-commands that are available")
    
    '''
    @see: AbstractPlugin
    '''    
    def detect_shortcut(self, args):
        
        return None