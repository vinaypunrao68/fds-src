from test.base_cli_test import BaseCliTest
from model.fds_id import FdsId
from model.volume.volume import Volume
from model.admin.user import User
from model.admin.role import Role
from model.admin.tenant import Tenant
from model.volume.volume_status import VolumeStatus
from model.common.size import Size
from model.volume.settings.object_settings import ObjectSettings
from model.volume.qos_policy import QosPolicy
from model.volume.data_protection_policy import DataProtectionPolicy
from model.common.duration import Duration
from model.volume.snapshot_policy import SnapshotPolicy
from model.volume.recurrence_rule import RecurrenceRule
from utils.converters.volume.volume_converter import VolumeConverter
from utils.converters.admin.user_converter import UserConverter
from utils.converters.admin.tenant_converter import TenantConverter
from model.platform.domain import Domain
from utils.converters.platform.domain_converter import DomainConverter
from model.platform.service import Service
from model.platform.service_status import ServiceStatus
from utils.converters.platform.service_converter import ServiceConverter
from mock import self
from model.platform.node import Node
from utils.converters.platform.node_converter import NodeConverter

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
        
    def test_user_conversion(self):
        
        user = User()
        role = Role()
        role.name = "USER"
        role.features = ["VOL_MGMT", "SYSTEM_MGMT"]

        tenant_id = FdsId()
        tenant_id.name = "TheWorst"
        tenant_id.uuid = 2

        ident = FdsId()
        
        ident.name = "jdoe"
        ident.uuid = "5abc34"
        
        user.id = ident
        user.tenant_id = tenant_id
        user.role = role
        
        j_user = UserConverter.to_json(user)
        print j_user
        
        new_user = UserConverter.build_user_from_json(j_user)
        
        assert new_user.id.uuid == "5abc34"
        assert new_user.id.name == "jdoe"
        assert new_user.role.name == "USER"
        assert len(new_user.role.features) == 2
        assert new_user.tenant_id.name == "TheWorst"
        assert new_user.tenant_id.uuid == 2
        
    def test_tenant_conversion(self):
        
        tenant = Tenant()
        
        ident = FdsId()
        ident.uuid = 320
        ident.name = "HisTenant"
        
        tenant.id = ident
        
        j_tenant = TenantConverter.to_json(tenant)
        print j_tenant
        
        new_tenant = TenantConverter.build_tenant_from_json(j_tenant)
        
        assert new_tenant.id.name == "HisTenant"
        assert new_tenant.id.uuid == 320
        
    def test_domain_conversion(self):
        
        domain = Domain()
        domain.site = "boulder"
        
        ident = FdsId()
        ident.uuid = 5102
        ident.name = "terrible.domain"
        
        domain.id = ident
        
        j_domain = DomainConverter.to_json(domain)
        print j_domain
        
        new_domain = DomainConverter.build_domain_from_json(j_domain)
        
        assert new_domain.site == "boulder"
        assert new_domain.id.name == "terrible.domain"
        assert new_domain.id.uuid == 5102
        
    def test_service_conversion(self):
        
        service = Service()
        
        ident = FdsId()
        ident.uuid = "qwerty"
        ident.name = "AM"
        
        service.id = ident
        
        service.port = 7004
        service.type = "FDSP_ACCESS_MGR"
        
        status = ServiceStatus()
        status.state = "LIMITED"
        status.description = "Some fake description"
        status.error_code = 4001
        
        service.status = status
        
        j_service = ServiceConverter.to_json(service)
        print j_service
        
        new_service = ServiceConverter.build_service_from_json(j_service)
        
        assert new_service.id.uuid == "qwerty"
        assert new_service.id.name == "AM"
        assert new_service.type == "FDSP_ACCESS_MGR"
        
        new_status = new_service.status
        
        assert new_status.state == "LIMITED"
        assert new_status.description == "Some fake description"
        assert new_status.error_code == 4001
        
    def test_node_conversion(self):
        
        node  = Node()
        
        node.ip_v4_address = "10.12.13.14"
        node.ip_v6_address = "someweirdaddress"
        
        ident = FdsId()
        ident.uuid = 928
        ident.name = "CoolNode"
        
        node.id = ident
        
        node.state = "DOWN"
        
        service = Service()
        
        s_ident = FdsId()
        s_ident.uuid = 21
        s_ident.name = "DM"
        service.id = s_ident
        
        service.type = "FDSP_DATA_MGR"
        
        s_status = ServiceStatus()
        s_status.description = "Doing very medium"
        s_status.error_code = 201
        s_status.state = "DEGRADED"
        
        service.status = s_status
        
        node.services["DM"].append( service )
        
        j_node = NodeConverter.to_json(node)
        print j_node
        
        new_node = NodeConverter.build_node_from_json(j_node)
        
        assert new_node.ip_v4_address == "10.12.13.14"
        assert new_node.ip_v6_address == "someweirdaddress"
        assert new_node.id.uuid == 928
        assert new_node.id.name == "CoolNode"
        assert new_node.state == "DOWN"
        assert len(new_node.services["DM"]) == 1
        
        new_service = new_node.services["DM"][0]
        
        assert new_service.status.state == "DEGRADED"
        assert new_service.status.description == "Doing very medium"
        assert new_service.status.error_code == 201
        assert new_service.type == "FDSP_DATA_MGR"
        assert new_service.id.uuid == 21
        assert new_service.id.name == "DM"

        