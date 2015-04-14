import abstract_plugin

'''
Created on Apr 13, 2015

Configure all the parsing for the stats queries and map those options
to the appropriate REST calls

@author: nate
'''
class StatisticsPlugin( abstract_plugin.AbstractPlugin):
    
    '''
    @see: AbstractPlugin
    '''
    def build_parser(self, parentParser, session): 
        
        self.__parser = parentParser.add_parser( "stats", help="Gather statistics from the system" )
        self.__subparser = self.__parser.add_subparsers( help="The sub-commands that are available")
        
    '''
    @see: AbstractPlugin
    '''    
    def detect_shortcut(self, args):
        
        return None