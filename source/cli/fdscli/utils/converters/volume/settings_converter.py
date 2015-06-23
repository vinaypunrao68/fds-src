import json
from model.volume.settings.block_settings import BlockSettings
from utils.converters.common.size_converter import SizeConverter
from model.volume.settings.object_settings import ObjectSettings

class SettingsConverter(object):
    '''
    Created on Jun 1, 2015
    
    @author: nate
    '''

    @staticmethod
    def to_json(settings):
        
        
        j_settings = dict()
        j_settings["type"] = settings.type
        
        if ( settings.type is "BLOCK" ):
            
            j_settings["capacity"] = json.loads(SizeConverter.to_json(settings.capacity))
            
            if settings.block_size is not None:
                j_settings["blockSize"] = json.loads(SizeConverter.to_json(settings.block_size))
            
        else:
            
            if settings.max_object_size is not None:
                j_settings["maxObjectSize"] = json.loads(SizeConverter.to_json(settings.max_object_size))
            
        j_settings = json.dumps(j_settings)
        
        return j_settings
    
    @staticmethod
    def build_settings_from_json(j_str):
        
        settings = None
        
        volume_type = j_str.pop( "type" )
        
        if volume_type == "BLOCK":
            
            settings = BlockSettings()
            
            if "blockSize" in j_str:
                settings.block_size = SizeConverter.build_size_from_json( j_str.pop("blockSize", settings.block_size) )
                
            settings.capacity = SizeConverter.build_size_from_json( j_str.pop("capacity", settings.capacity) )
            
        else:
            
            settings = ObjectSettings()
            
            if "maxObjectSize" in j_str:
                settings.max_object_size = SizeConverter.build_size_from_json( j_str.pop("maxObjectSize", settings.max_object_size) )
            
        return settings
