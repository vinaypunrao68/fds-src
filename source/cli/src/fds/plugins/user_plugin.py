import abstract_plugin

'''
Created on Apr 13, 2015

Plugin to handle the parsing of all user related tasks and the calls
to the corresponding REST endpoints to manage users

@author: nate
'''
class UserPlugin( abstract_plugin.AbstractPlugin):
    
    def __init__(self, session):
        abstract_plugin.AbstractPlugin.__init__(self, session)    
    
    '''
    @see: AbstractPlugin
    '''
    def build_parser(self, parentParser, session): 
        
        self.__parser = parentParser.add_parser( "user", help="Manage FDS users" )
        self.__subparser = self.__parser.add_subparsers( help="The sub-commands that are available")
    
    '''
    @see: AbstractPlugin
    '''    
    def detect_shortcut(self, args):
        
        return None