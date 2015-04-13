'''
Created on Apr 13, 2015

@author: nate
'''

import abstract_plugin

class LocalDomainModule( abstract_plugin.AbstractPlugin):
    
    def build_parser(self, parentParser, session): 
        
        self.__parser = parentParser.add_parser( "local_domain", help="Manage and interact with local domains" )
        self.__subparser = self.__parser.add_subparsers( help="The sub-commands that are available")
        
    def detect_shortcut(self, args):
        
        return None