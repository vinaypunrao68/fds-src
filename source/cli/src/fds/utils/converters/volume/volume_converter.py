import json
from fds.model.volume.volume import Volume
from fds.utils.converters.fds_id_converter import FdsIdConverter
from fds.utils.converters.volume.qos_policy_converter import QosPolicyConverter
from fds.utils.converters.volume.settings_converter import SettingsConverter
from fds.utils.converters.volume.volume_status_converter import VolumeStatusConverter
from fds.utils.converters.volume.data_protection_policy_converter import DataProtectionPolicyConverter

class VolumeConverter( object ):
    '''
    Created on Apr 14, 2015
    
    Because we plan on using generated object models we needed a separate mechanism for marshalling
    volume objects between the python class representation and the JSON dictionary objects.  
    
    This is that mechanism
    
    @author: nate
    '''
        
    @staticmethod
    def build_volume_from_json( j_str ):
        '''
        This takes a json dictionary and turns it into a volume class instantiation
        '''

        volume = Volume();
        
        if not isinstance( j_str, dict ):
            j_str = json.loads(j_str)
        
        volume.id = FdsIdConverter.build_id_from_json(j_str.pop("id"))
        volume.status = VolumeStatusConverter.build_status_from_json(j_str.pop("status"))
        volume.media_policy = j_str.pop("media_policy", volume.media_policy)
        volume.application = j_str.pop("application", volume.application)
        volume.qos_policy = QosPolicyConverter.build_policy_from_json(j_str.pop("qos_policy"))
        volume.data_protection_policy = DataProtectionPolicyConverter.build_policy_from_json(j_str.pop("data_protection_policy"))
        volume.tenant_id = FdsIdConverter.build_id_from_json(j_str.pop("tenant_id"))
        
        return volume
    
    @staticmethod
    def to_json( volume ):
        '''
        This will take a volume object and turn it into a JSON dictionary object compatible with the
        json module
        '''
        
        d = dict()
        
        j_id = FdsIdConverter.to_json( volume.id )
        
        d["id"] = json.loads(j_id)
        
        j_qos = QosPolicyConverter.to_json(volume.qos_policy)
        
        d["qos_policy"] = json.loads(j_qos)
        d["media_policy"] = volume.media_policy

        j_settings = SettingsConverter.to_json(volume.settings)
        d["settings"] = json.loads(j_settings)

        j_status = VolumeStatusConverter.to_json(volume.status)
        d["status"] = json.loads(j_status)
        
        j_d_policy = DataProtectionPolicyConverter.to_json(volume.data_protection_policy)
        d["data_protection_policy"] = json.loads(j_d_policy)
        
        d["application"] = volume.application
        d["tenant_id"] = json.loads(FdsIdConverter.to_json(volume.tenant_id))
        
        result = json.dumps( d )
        
        return result
