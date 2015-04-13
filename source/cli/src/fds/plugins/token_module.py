'''
Created on Apr 13, 2015

@author: nate
'''
import abstract_plugin

class TokenModule( abstract_plugin.AbstractPlugin):
    
    def build_parser(self, parentParser, session): 
        
        self.__parser = parentParser.add_parser( "token", help="Interact with the authentication/authorization system" )
        self.__subparser = self.__parser.add_subparsers( help="The sub-commands that are available")
        
    def detect_shortcut(self, args):
        
        return None