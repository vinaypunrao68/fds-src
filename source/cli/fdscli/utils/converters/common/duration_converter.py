import json
from model.common.duration import Duration
class DurationConverter(object):
    '''
    Created on Jun 1, 2015
    
    @author: nate
    '''

    @staticmethod
    def to_json(duration):
        
        j_str = dict()
        
        j_str["time"] = duration.time
        j_str["unit"] = duration.unit
        
        j_str = json.dumps(j_str)
        
        return j_str
    
    @staticmethod
    def build_duration_from_json(j_str):
        
        duration = Duration()
        
        if not isinstance(j_str, dict):
            j_str  = json.loads(j_str)
        
        duration.time = j_str.pop("time", duration.time)
        duration.unit = j_str.pop("unit", duration.unit)
        
        return duration
