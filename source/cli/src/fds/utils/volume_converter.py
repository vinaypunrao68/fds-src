import json
from fds.model.volume import Volume

'''
Created on Apr 14, 2015

Because we plan on using generated object models we needed a separate mechanism for marshalling
volume objects between the python class representation and the JSON dictionary objects.  

This is that mechanism

@author: nate
'''
class VolumeConverter( object ):
    
    '''
    This takes a json dictionary and turns it into a volume class instantiation
    '''
    @staticmethod
    def build_volume_from_json( json ):

        volume = Volume();
        
        volume.name = json.pop( "name", None )
        volume.iops_guarantee = json.pop( "sla", int(volume.iops_guarantee) )
        volume.iops_limit = json.pop( "limit", int(volume.iops_limit) )
        volume.id = json.pop( "id", volume.id )
        volume.media_policy = json.pop( "mediaPolicy", volume.media_policy )
        volume.continuous_protection = json.pop( "commit_log_retention", int(volume.continuous_protection) )
        volume.priority = json.pop( "priority", int(volume.priority) )
        volume.tenant_id = json.pop( "tenantId", volume.tenant_id)
        
        dc = json.pop( "data_connector", None )
        
        if ( dc != None ):
            volume.type = dc.pop( "type", volume.type )
        
        volume.state = json.pop( "state", volume.state )
        
        cu = json.pop( "current_usage", None )
        
        if ( cu != None ):
            volume.current_size = cu.pop( "size", None )
            volume.current_units = cu.pop( "unit", None )
        
        return volume
    
    '''
    This will take a volume object and turn it into a JSON dictionary object compatible with the
    json module
    '''
    @staticmethod
    def to_json( volume ):
        
        d = dict()
        d["name"] = volume.name
        d["id"] = volume.id
        d["limit"] = volume.iops_limit
        d["sla"] = volume.iops_guarantee
        d["mediaPolicy"] = volume.media_policy
        d["priority"] = volume.priority
        d["commit_log_retention"] = volume.continuous_protection
        
        dc = dict()
        
        if ( volume.type == "object" ):
            dc["api"] = "S3, Swift"
            dc["type"] = "OBJECT"
        else:
            dc["api"] = "Basic, Cinder"
            dc["type"] = "BLOCK"
            attrs = dict()
            attrs["size"] = volume.current_size
            attrs["unit"] = volume.current_units
            dc["attributes"] = attrs
            
        d["data_connector"] = dc
        
        result = json.dumps( d )
        
        return result