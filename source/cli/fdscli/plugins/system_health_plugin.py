from .abstract_plugin import AbstractPlugin
from services.stats_service import StatsService
from utils.converters.health.system_health_converter import SystemHealthConverter
from services.response_writer import ResponseWriter
import json
from model.health.health_state import HealthState
from model.health.system_health import SystemHealth

class SystemHealthPlugin( AbstractPlugin):
    '''
    Created on Apr 13, 2015
    
    A plugin to gather the system health data and set the parsing up 
    for that operation
    
    @author: nate
    '''
    
    def __init__(self):
        AbstractPlugin.__init__(self)    
    
    '''
    @see: AbstractPlugin
    '''
    def build_parser(self, parentParser, session): 
        
        self.session = session
        
        self.__stat_service = StatsService(self.session)
        
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
        
        if not isinstance( health, SystemHealth ):        
            return
        
        if AbstractPlugin.format_str in args and args[AbstractPlugin.format_str] == "json":
            j_str = SystemHealthConverter.to_json(health)
            j_str = json.loads(j_str)
            
            ResponseWriter.writeJson(j_str)
        else:
            t_data = ResponseWriter.prep_system_health_for_table(health)
            ResponseWriter.writeTabularData(t_data)
            
            for record in health.health_records:
                print(record.category + ": " + HealthState.MESSAGES[record.message])
                
            print("")
        