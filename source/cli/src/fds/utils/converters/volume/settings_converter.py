import json
from fds.model.volume.settings.block_settings import BlockSettings
from fds.utils.converters.common.size_converter import SizeConverter
from fds.model.volume.settings.object_settings import ObjectSettings

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
            
            j_settings["capacity"] = json.loads(SizeConverter.to_json(settings.size))
            j_settings["blockSize"] = json.loads(SizeConverter.to_json(settings.block_size))
            
        else:
            
            j_settings["maxObjectSize"] = json.loads(SizeConverter.to_json(settings.max_object_size))
            
        j_settings = json.dumps(j_settings)
        
        return j_settings
    
    @staticmethod
    def build_settings_from_json(j_str):
        
        settings = None
        
        if j_str.pop("type") is "BLOCK":
            
            settings = BlockSettings()
            settings.block_size = SizeConverter.build_size_from_json( settings.pop("blockSize") )
            settings.capacity = SizeConverter.build_size_from_json( settings.pop("capacity") )
            
        else:
            
            settings = ObjectSettings()
            settings.max_object_size = SizeConverter.build_size_from_json( j_str.pop("maxObjectSize", settings.max_object_size) )
            
        return settings