import abstract_plugin

'''
Created on Apr 13, 2015

This plugin configures all of the parsing for tenant specific operation and
maps those options to the appropriate calls for tenant management

@author: nate
'''
class TenantPlugin( abstract_plugin.AbstractPlugin):
    
    def __init__(self, session):
        abstract_plugin.AbstractPlugin.__init__(self, session)    
    
    '''
    @see: AbstractPlugin
    '''
    def build_parser(self, parentParser, session): 
        
        self.__parser = parentParser.add_parser( "tenant", help="Manage tenants of the system" )
        self.__subparser = self.__parser.add_subparsers( help="The sub-commands that are available")
    
    '''
    @see: AbstractPlugin
    '''    
    def detect_shortcut(self, args):
        
        return None
