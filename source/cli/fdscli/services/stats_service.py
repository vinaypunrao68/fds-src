from .abstract_service import AbstractService
from utils.converters.statistics.metric_query_converter import MetricQueryConverter
from utils.converters.statistics.statistics_converter import StatisticsConverter
from utils.converters.health.system_health_converter import SystemHealthConverter
from utils.converters.statistics.firebreak_query_converter import FirebreakQueryConverter
from model.fds_error import FdsError

class StatsService( AbstractService ):
    '''
    Created on Apr 23, 2015
    
    @author: nate
    '''

    def __init__(self, session):
        AbstractService.__init__(self, session)
        
    def query_volume_metrics(self, metrics_filter):
        '''
        Make a query to the system metrics based on the filter criteria passed in
        '''
        
        url = "{}{}".format( self.get_url_preamble(), "/stats/volumes")
        #TODO convert filter to json
        data = MetricQueryConverter.to_json(metrics_filter)
        stat_json = self.rest_helper.put( self.session, url, data)
        
        if isinstance(stat_json, FdsError):
            return stat_json
        
        stats = StatisticsConverter.build_statistics_from_json(stat_json)
        
        return stats
    
    def query_firebreak_metrics(self, metrics_filter):
        '''
        Query the firebreak metrics based on the filter criteria passed in
        '''
        
        url = "{}{}".format( self.get_url_preamble(), "/stats/volumes/firebreak" )
        #TODO convert the filter to JSON
        data = FirebreakQueryConverter.to_json(metrics_filter)
        stats = self.rest_helper.put( self.session, url, data)
        
        if isinstance(stats, FdsError):
            return stats
        
        stats = StatisticsConverter.build_statistics_from_json(stats)
        
        return stats
    
    def get_system_health_report(self):
        '''
        Get the overall system health report
        '''
        
        url = "{}{}".format( self.get_url_preamble(), "/systemhealth" )
        sys_health = self.rest_helper.get( self.session, url )
        
        if isinstance(sys_health, FdsError):
            return  sys_health
        
        sys_health  = SystemHealthConverter.build_system_health_from_json(sys_health)
        
        return sys_health