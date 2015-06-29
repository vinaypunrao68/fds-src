from abstract_plugin import AbstractPlugin
import time
from model.statistics.metric_query_criteria import MetricQueryCriteria
from model.statistics.date_range import DateRange
from services.volume_service import VolumeService
from model.statistics.metrics import Metric
from services.stats_service import StatsService
from services.response_writer import ResponseWriter
from model.statistics.statistics import Statistics
from utils.converters.statistics.statistics_converter import StatisticsConverter
import json

class StatisticsPlugin( AbstractPlugin):
    '''
    Created on Apr 13, 2015
    
    Configure all the parsing for the stats queries and map those options
    to the appropriate REST calls
    
    @author: nate
    '''
    def __init__(self, session):
        AbstractPlugin.__init__(self, session)
    
    '''
    @see: AbstractPlugin
    '''
    def build_parser(self, parentParser, session): 
        
        self.__stats_service = StatsService( session )
        
        self.__parser = parentParser.add_parser( "stats", help="Gather statistics from the system" )
        self.__subparser = self.__parser.add_subparsers( help="The sub-commands that are available")
        
        self.create_volume_query_parser( self.__subparser )
        
    def create_volume_query_parser(self, subparser):
        
        query_parser = subparser.add_parser( "volumes", help="Submit a query to the FDS system.")
        
        query_parser.add_argument( self.arg_str + AbstractPlugin.volume_ids_str, help="A list of volume IDs to include in this query.  If omitted, it will include all volumes.", nargs="+", type=int, required=False)
        query_parser.add_argument( self.arg_str + AbstractPlugin.metrics_str, help="A list of metrics to return with this query.", nargs="+",  required=True)
        query_parser.add_argument( self.arg_str + AbstractPlugin.start_str, help="A time (in seconds from epoch) where you would like the query to begin from.", type=int, default=0)
        query_parser.add_argument( self.arg_str + AbstractPlugin.end_str, help="A time (in seconds from epoch) where you would like the query to end.", type=int, default=(time.time()/1000.0))
        query_parser.add_argument( self.arg_str + AbstractPlugin.points_str, help="The number of datapoints that you wish to receive from this query.", type=int, default=None)
        
        query_parser.set_defaults( func=self.query_volumes )
        
    def create_firebreak_query_parser(self, subparser):
        
        fb_parser = subparser.add_parser( "firebreak", help="Submit a query for firebreak information")
        
    '''
    @see: AbstractPlugin
    '''    
    def detect_shortcut(self, args):
        
        return None
    
    def get_stats_service(self):
        return self.__stats_service
    
    def query_volumes(self, args):
        '''
        Create the filter object to use in our query
        '''
        
        query = MetricQueryCriteria()
        date_range = DateRange()
        
        date_range.start = args[AbstractPlugin.start_str]
        date_range.end = args[AbstractPlugin.end_str]
        
        if args[AbstractPlugin.volume_ids_str] is not None:
            for vol_id in args[AbstractPlugin.volume_ids_str]:
                volume = VolumeService().get_volume( vol_id )
                query.contexts.append( volume )
                
        if args[AbstractPlugin.points_str] is not None:
            query.points = args[AbstractPlugin.points_str]
            
        for metric_name in args[AbstractPlugin.metrics_str]:
            metric = Metric.DICT[metric_name]
            query.metrics.append( metric )
            
        stats = self.get_stats_service().query_volume_metrics( query )
        
        if isinstance(stats, Statistics):
            j_str = StatisticsConverter.to_json( stats )
            j_str = json.loads(j_str)
            ResponseWriter.writeJson( j_str )
        