from .abstract_plugin import AbstractPlugin

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
from model.statistics.firebreak_query_criteria import FirebreakQueryCriteria
from services.fds_auth import FdsAuth

class StatisticsPlugin( AbstractPlugin):
    '''
    Created on Apr 13, 2015
    
    Configure all the parsing for the stats queries and map those options
    to the appropriate REST calls
    
    @author: nate
    '''
    def __init__(self):
        AbstractPlugin.__init__(self)
    
    '''
    @see: AbstractPlugin
    '''
    def build_parser(self, parentParser, session): 
        
        self.session = session
        
        if not self.session.is_allowed( FdsAuth.VOL_MGMT ):
            return
        
        self.__stats_service = StatsService( self.session )
        self.__vol_service = VolumeService( self.session )
        
        self.__parser = parentParser.add_parser( "stats", help="Gather statistics from the system" )
        self.__subparser = self.__parser.add_subparsers( help="The sub-commands that are available")
        
        self.create_volume_query_parser( self.__subparser )
        self.create_firebreak_query_parser( self.__subparser )
        
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
        
        fb_parser.add_argument( self.arg_str + AbstractPlugin.volume_ids_str, help="A list of volume IDs to include in this query.  If omitted, it will include all volumes.", nargs="+", type=int, required=False)
        fb_parser.add_argument( self.arg_str + AbstractPlugin.start_str, help="A time (in seconds from epoch) where you would like the query to begin from.", type=int, default=0)
        fb_parser.add_argument( self.arg_str + AbstractPlugin.end_str, help="A time (in seconds from epoch) where you would like the query to end.", type=int, default=(time.time()/1000.0))
        fb_parser.add_argument( self.arg_str + AbstractPlugin.most_recent_str, help="The number of firebreak occurrences you would like to include.", type=int, default=1)
        fb_parser.add_argument( self.arg_str + AbstractPlugin.size_for_value_str, help="If true, the y value will be the size of the volume instead of the raw number.", choices=["true", "false"], default="true")
        
        fb_parser.set_defaults( func=self.query_firebreaks )
        
    '''
    @see: AbstractPlugin
    '''    
    def detect_shortcut(self, args):
        
        return None
    
    def get_stats_service(self):
        return self.__stats_service
    
    def get_volume_service(self):
        return self.__vol_service
    
    def build_base_query(self, query, args):
        '''
        Helper method to build out the basic query since most have the same major components
        '''
        
        date_range = DateRange()
        
        date_range.start = args[AbstractPlugin.start_str]
        date_range.end = args[AbstractPlugin.end_str]
        
        if AbstractPlugin.volume_ids_str in args and args[AbstractPlugin.volume_ids_str] is not None:
            for vol_id in args[AbstractPlugin.volume_ids_str]:
                volume = self.get_volume_service().get_volume( vol_id )
                query.contexts.append( volume )
                
        if AbstractPlugin.points_str in args and args[AbstractPlugin.points_str] is not None:
            query.points = args[AbstractPlugin.points_str]
            
        if AbstractPlugin.metrics_str in args:
            for metric_name in args[AbstractPlugin.metrics_str]:
                metric = Metric.DICT[metric_name]
                query.metrics.append( metric )
            # end of for
        #fi
            
        return query
    
    def query_volumes(self, args):
        '''
        Create the filter object to use in our query
        '''
        
        query = MetricQueryCriteria()
        query = self.build_base_query(query, args)
            
        stats = self.get_stats_service().query_volume_metrics( query )
        
        if isinstance(stats, Statistics):
            j_str = StatisticsConverter.to_json( stats )
            j_str = json.loads(j_str)
            ResponseWriter.writeJson( j_str )
            
    def query_firebreaks(self, args):
        '''
        create the firebreak query to use in our request
        '''
        
        query = FirebreakQueryCriteria()
        query = self.build_base_query(query, args)
        
        use_size = True
        
        if args[AbstractPlugin.size_for_value_str] == "false":
            use_size = False
        
        query.use_size_for_value = use_size
        query.points = args[AbstractPlugin.most_recent_str]
        
        stats = self.get_stats_service().query_firebreak_metrics(query)
        
        if isinstance(stats, Statistics):
            j_str = StatisticsConverter.to_json( stats )
            j_str = json.loads(j_str)
            ResponseWriter.writeJson( j_str )        
        