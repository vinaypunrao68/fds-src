from . import abstract_plugin

'''
Created on Apr 13, 2015

Plugin responsible for all parsing for token related operations
and starting the interactions with the REST services regarding 
token management

@author: nate
'''
class TokenPlugin( abstract_plugin.AbstractPlugin):
    
    def __init__(self):
        abstract_plugin.AbstractPlugin.__init__(self)    
    
    '''
    @see: AbstractPlugin
    '''
    def build_parser(self, parentParser, session): 
        
        self.session = session
        
        self.__parser = parentParser.add_parser( "token", help="Interact with the authentication/authorization system" )
        self.__subparser = self.__parser.add_subparsers( help="The sub-commands that are available")
    
    '''
    @see: AbstractPlugin
    '''    
    def detect_shortcut(self, args):
        
        return None