from model.statistics.metric_query_criteria import MetricQueryCriteria
from model.volume.volume import Volume
from utils.converters.volume.volume_converter import VolumeConverter
import json
from utils.converters.statistics.date_range_converter import DateRangeConverter

class MetricQueryConverter(object):
    '''
    Created on Jun 29, 2015
    
    @author: nate
    '''

    @staticmethod
    def to_json(query):
        
#         if not isinstance(query, MetricQueryCriteria):
#             raise TypeError("Must be a metric query criteria.")
#         
        q_json = dict()
        
        context_array = []
        
        for context in query.contexts:
            if isinstance(context, Volume):
                context_array.append( json.loads(VolumeConverter.to_json(context)))
        
        q_json["contexts"] = context_array
        q_json["range"] = json.loads( DateRangeConverter.to_json(query.date_range) )
        
        metrics = []
        
        for metric in query.metrics:
            metrics.append( metric[0] )
            
        q_json["seriesType"] = metrics
        
        if query.points is not None:
            q_json["points"] = query.points
        
        q_json = json.dumps(q_json)
        
        return q_json