from . import abstract_plugin

'''
Created on Apr 13, 2015

A plugin to return information about the global domain.  This will
set up both the parsing and the routing to the appropriate functions or
REST calls

@author: nate
'''
from services.fds_auth import FdsAuth
class GlobalDomainPlugin( abstract_plugin.AbstractPlugin):
    
    def __init__(self):
        abstract_plugin.AbstractPlugin.__init__(self)    
    
    '''
    @see: AbstractPlugin
    '''
    def build_parser(self, parentParser, session): 
        
        self.session = session
        
        if not session.is_allowed( FdsAuth.SYS_MGMT ):
            return
        
        self.__parser = parentParser.add_parser( "global_domain", help="Interact with the global domain" )
        self.__subparser = self.__parser.add_subparsers( help="The sub-commands that are available")
    
    '''
    @see: AbstractPlugin
    '''    
    def detect_shortcut(self, args):
        
        return None
