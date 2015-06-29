import abstract_plugin

'''
Created on Apr 13, 2015

A plugin to gather the system health data and set the parsing up 
for that operation

@author: nate
'''
from services.stats_service import StatsService
class SystemHealthPlugin( abstract_plugin.AbstractPlugin):
    
    def __init__(self, session):
        abstract_plugin.AbstractPlugin.__init__(self, session)    
    
    '''
    @see: AbstractPlugin
    '''
    def build_parser(self, parentParser, session): 
        
        self.__stat_service = StatsService(session)
        
        self.__parser = parentParser.add_parser( "system_health", help="Query the system health" )
        
        self.add_format_arg(self.__parser)
        
        self.__parser.set_defaults( func=self.get_health, format="tabular")
    
    '''
    @see: AbstractPlugin
    '''    
    def detect_shortcut(self, args):
        
        return None
    
    def get_stat_service(self):
        return self.__stat_service
    
    def get_health(self, args):
        '''
        get the overall health of the system
        '''
        health = self.get_stat_service().get_system_health_report()
        
        
        