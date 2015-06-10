from utils.converters.common.size_converter import SizeConverter
import json
from model.volume.volume_status import VolumeStatus

class VolumeStatusConverter(object):
    '''
    Created on Jun 1, 2015
    
    @author: nate
    '''

    @staticmethod
    def to_json(status):
        
        j_str = dict()
        
        j_str["state"] = status.state
        
        capFb = dict()
        capFb["seconds"] = status.last_capacity_firebreak
        capFb["nanos"] = 0
        
        perfFb = dict()
        perfFb["seconds"] = status.last_performance_firebreak
        perfFb["nanos"] = 0
        
        j_str["lastCapacityFirebreak"] = capFb
        j_str["lastPerformanceFirebreak"] = perfFb
        j_str["currentUsage"] = json.loads(SizeConverter.to_json(status.current_usage))
        
        j_str = json.dumps(j_str)
        
        return j_str
    
    @staticmethod
    def build_status_from_json(j_str):
        
        status = VolumeStatus()
        
        status.state = j_str.pop("state", status.state)
        
        capFb = j_str.pop("lastCapacityFirebreak", status.last_capacity_firebreak)
        status.last_capacity_firebreak = capFb["seconds"]
        perfFb = j_str.pop("lastPerformanceFirebreak", status.last_performance_firebreak)
        status.last_performance_firebreak = perfFb["seconds"]
        status.current_usage = SizeConverter.build_size_from_json(j_str.pop("currentUsage"))
        
        return status
