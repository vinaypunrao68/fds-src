from model.volume.volume import Volume
from utils.converters.volume.volume_converter import VolumeConverter
import json
from utils.converters.statistics.date_range_converter import DateRangeConverter
from model.statistics.firebreak_query_criteria import FirebreakQueryCriteria

class FirebreakQueryConverter(object):
    '''
    Created on Jun 30, 2015
    
    @author: nate
    '''
    
    @staticmethod
    def to_json(query):
        
#         if not isinstance(query, FirebreakQueryCriteria):
#             raise TypeError()
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
        q_json["useSizeForValue"] = query.use_size_for_value
        
        if query.points is not None:
            q_json["mostRecentResults"] = query.points
        
        q_json = json.dumps(q_json)
        
        return q_json