'''
Created on Apr 13, 2015

@author: nate
'''
import abstract_plugin

class EventModule( abstract_plugin.AbstractPlugin):
    
    def build_parser(self, parentParser, session): 
        
        self.__parser = parentParser.add_parser( "event", help="Gather system events" )
        self.__subparser = self.__parser.add_subparsers( help="The sub-commands that are available")
        
    def detect_shortcut(self, args):
        
        return None