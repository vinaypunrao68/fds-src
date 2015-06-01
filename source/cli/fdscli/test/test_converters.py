from test.base_cli_test import BaseCliTest
from model.fds_id import FdsId
from model.volume.volume import Volume
from model.volume.volume_status import VolumeStatus
from model.common.size import Size
from model.volume.settings.object_settings import ObjectSettings
from model.volume.qos_policy import QosPolicy
from model.volume.data_protection_policy import DataProtectionPolicy
from model.common.duration import Duration
from model.volume.snapshot_policy import SnapshotPolicy
from model.volume.recurrence_rule import RecurrenceRule
from utils.converters.volume.volume_converter import VolumeConverter

class TestConverters(BaseCliTest):
    '''
    Created on Jun 1, 2015
    
    @author: nate
    '''
    
    def test_volume_conversion(self):
        
        volume = Volume()
        an_id = FdsId()
        an_id.uuid = 34
        an_id.name = "TestVolume"
        volume.id = an_id  
        volume.media_policy = "SSD_ONLY"
        
        status = VolumeStatus()
        status.last_capacity_firebreak = 0
        status.last_performance_firebreak = 10000
        status.state = "AVAILABLE"
        status.current_usage = Size( 12, "TB" )
        volume.status = status
        volume.application = "MS Access"
        
        tenant_id = FdsId()
        tenant_id.uuid = 9
        tenant_id.name = "UNC"
        volume.tenant_id = tenant_id
        
        settings = ObjectSettings()
        settings.max_object_size = Size( 1, "GB" )
        volume.settings = settings
        
        qos_policy = QosPolicy()
        qos_policy.priority = 6
        qos_policy.iops_max = 5000
        qos_policy.iops_min = 3000
        volume.qos_policy = qos_policy
        
        p_policy = DataProtectionPolicy()
        p_policy.commit_log_retention = Duration(1, "DAYS")
        p_policy.preset_id = None
        
        s_policy = SnapshotPolicy()
        s_policy.retention_time_in_seconds = 86400
        s_policy.preset_id = None
        
        rule = RecurrenceRule()
        rule.byday = "MO"
        rule.frequency = "WEEKLY"
        rule.byhour = 14
        rule.byminute = 45
        s_policy.recurrence_rule = rule
        
        p_policy.snapshot_policies = [s_policy]
        volume.data_protection_policy = p_policy 
        
        j_str = VolumeConverter.to_json(volume) 
        
        newVolume = VolumeConverter.build_volume_from_json(j_str)
        
        print j_str
        
        assert newVolume.id.name == "TestVolume"
        assert newVolume.id.uuid == 34
        assert newVolume.media_policy == "SSD_ONLY"
        assert newVolume.status.state == "AVAILABLE"
        assert newVolume.status.last_capacity_firebreak == 0
        assert newVolume.status.last_performance_firebreak == 10000
        assert newVolume.status.current_usage.size == 12
        assert newVolume.status.current_usage.unit == "TB"
        assert newVolume.application == "MS Access"
        assert newVolume.qos_policy.priority == 6
        assert newVolume.qos_policy.iops_max == 5000
        assert newVolume.qos_policy.iops_min == 3000
        assert newVolume.data_protection_policy.commit_log_retention.time == 1
        assert newVolume.data_protection_policy.commit_log_retention.unit == "DAYS"
        assert len(newVolume.data_protection_policy.snapshot_policies) == 1
        assert newVolume.tenant_id.uuid == 9
        assert newVolume.tenant_id.name == "UNC"
        
        new_s_policy = newVolume.data_protection_policy.snapshot_policies[0]
        
        assert new_s_policy.retention_time_in_seconds == 86400
        assert new_s_policy.recurrence_rule.frequency == "WEEKLY"
        assert new_s_policy.recurrence_rule.byday == "MO"
        assert new_s_policy.recurrence_rule.byhour == 14
        assert new_s_policy.recurrence_rule.byminute == 45

