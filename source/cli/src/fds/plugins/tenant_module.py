'''
Created on Apr 13, 2015

@author: nate
'''
import abstract_plugin

class TenantModule( abstract_plugin.AbstractPlugin):
    
    def build_parser(self, parentParser, session): 
        
        self.__parser = parentParser.add_parser( "tenant", help="Manage tenants of the system" )
        self.__subparser = self.__parser.add_subparsers( help="The sub-commands that are available")
        
    def detect_shortcut(self, args):
        
        return None
