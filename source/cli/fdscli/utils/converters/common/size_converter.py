import json
from model.common.size import Size

class SizeConverter(object):
    '''
    Created on Jun 1, 2015
    
    @author: nate
    '''

    @staticmethod
    def to_json(size):
        
        j_str = dict()
        
        j_str["value"] = size.size
        j_str["unit"] = size.unit
        
        j_str = json.dumps(j_str)
        
        return j_str
        
    @staticmethod
    def build_size_from_json(j_str):
        
        if not isinstance(j_str, dict):
            j_str = json.loads(j_str)
            
        
        size = Size()
        
        size.size = j_str.pop("value", size.size)
        size.unit = j_str.pop("unit", size.unit)
        
        return size
