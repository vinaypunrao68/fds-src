from abstract_service import AbstractService
from utils.converters.statistics.metric_query_converter import MetricQueryConverter
from utils.converters.statistics.statistics_converter import StatisticsConverter

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
        
        if stat_json is None:
            return
        
        stats = StatisticsConverter.build_statistics_from_json(stat_json)
        
        return stats
    
    def query_firebreak_metrics(self, metrics_filter):
        '''
        Query the firebreak metrics based on the filter criteria passed in
        '''
        
        url = "{}{}".format( self.get_url_preamble(), "/stats/volumes/firebreak" )
        #TODO convert the filter to JSON
        data = ""
        return self.rest_helper.put( self.session, url, data)
    
    def get_system_health_report(self):
        '''
        Get the overall system health report
        '''
        
        url = "{}{}".format( self.get_url_preamble(), "/systemhealth" )
        return self.rest_helper().get( self.session, url )