'''
Created on Apr 13, 2015

@author: nate
'''
import abstract_plugin

class GlobalDomainModule( abstract_plugin.AbstractPlugin):
    
    def build_parser(self, parentParser, session): 
        
        self.__parser = parentParser.add_parser( "global_domain", help="Interact with the global domain" )
        self.__subparser = self.__parser.add_subparsers( help="The sub-commands that are available")
        
    def detect_shortcut(self, args):
        
        return None
