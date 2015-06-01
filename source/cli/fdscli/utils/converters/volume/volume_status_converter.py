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
        j_str["last_capacity_firebreak"] = status.last_capacity_firebreak
        j_str["last_performance_firebreak"] = status.last_performance_firebreak
        j_str["current_usage"] = SizeConverter.to_json(status.current_usage)
        
        j_str = json.dumps(j_str)
        
        return j_str
    
    @staticmethod
    def build_status_from_json(j_str):
        
        status = VolumeStatus()
        
        status.state = j_str.pop("state", status)
        status.last_capacity_firebreak = j_str.pop("last_capacity_firebreak", status.last_capacity_firebreak)
        status.last_performance_firebreak = j_str.pop("last_performance_firebreak", status.last_performance_firebreak)
        status.current_usage = SizeConverter.build_size_from_json(j_str.pop("current_usage"))
        
        return status