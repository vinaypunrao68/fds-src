from model.volume.volume import Volume
from model.volume.snapshot import Snapshot
from model.platform.node import Node
from model.platform.service import Service
from model.platform.domain import Domain
from model.volume.snapshot_policy import SnapshotPolicy
from model.volume.qos_preset import QosPreset
from model.admin.user import User
from model.admin.tenant import Tenant
from model.volume.data_protection_policy_preset import DataProtectionPolicyPreset
from model.platform.address import Address
from model.health.system_health import SystemHealth
from model.health.health_record import HealthRecord
from model.health.health_state import HealthState
from model.health.health_category import HealthCategory
from model.statistics.statistics import Statistics
from model.statistics.series import Series
from model.statistics.datapoint import Datapoint
from model.statistics.calculated import Calculated
from utils.converters.platform.node_converter import NodeConverter
import json

'''
Created on Apr 22, 2015

@author: nate
'''

responseOk = dict()
responseOk["status"] = "OK"

response200 = dict()
response200["status"] = 200

def passwordGetter(prompt):
    return "password"

def writeJson( data ):
    return

def listVolumes():
    volume = Volume(name="FakeVol",an_id=1)
    vols = []

    vols.append( volume )
    
    return vols

def createVolume(volume):
    return volume

def editVolume(volume):
    return volume

def cloneFromTimelineTime( volume, a_time ):
    
    volume.id = 354
    volume.timeline_time = a_time
    return volume

def cloneFromSnapshotId( volume, snapshot_id ):
    volume.id = 789
    return volume

def deleteVolume(name):
    return responseOk

def findVolumeById( an_id ):
    volume = Volume()
    volume.name = "VolumeName"
    volume.id = an_id
    
    return volume

def findVolumeByName( name ):
    volume = Volume()
    volume.name = name
    volume.id = 100
    
    return volume

def findVolumeBySnapId( an_id ):
    volume = Volume()
    volume.name = "SnapVol"
    volume.id = 300
    
    return volume

def createSnapshot( snapshot ):
    
    return snapshot

def listSnapshots( volumeName ):
    snapshot = Snapshot()
    snaps = []
    snaps.append( snapshot )
    return snaps

def listServices(nodeId):

    services = []
    services.append( Service(a_type="AM",name="AM") )
    services.append( Service(a_type="DM",name="DM") )
    services.append( Service(a_type="PM",name="PM") )
    services.append( Service(a_type="SM",name="SM") )
    
    return services  

def listNodes():
    node = Node()
    node.name = "FakeNode"
    address = Address()
    address.ipv4address = "10.12.14.15"
    address.ipv6address = "Ihavenoidea" 
    node.address = address
    node.id = "21ABC"
    node.state = "UP"
    
    node.services["AM"]  = [Service(a_type="AM",name="AM")]
    node.services["DM"]  = [Service(a_type="DM",name="DM")]
    node.services["PM"]  = [Service(a_type="PM",name="PM")]
    node.services["SM"]  = [Service(a_type="SM",name="SM")]                
 
    nodes = [node]
    return nodes

def listDiscoveredNodes():
    node = Node()
    node.name = "FakeNode"
    address = Address()
    address.ipv4address = "10.12.14.15"
    address.ipv6address = "Ihavenoidea" 
    node.address = address
    node.id = "21ABC"
    node.state = "DISCOVERED"
    
    node.services["AM"]  = [Service(a_type="AM",name="AM")]
    node.services["DM"]  = [Service(a_type="DM",name="DM")]
    node.services["PM"]  = [Service(a_type="PM",name="PM")]
    node.services["SM"]  = [Service(a_type="SM",name="SM")]                
 
    nodes = [node]
    return nodes

def addNode( node_id, state ):
    return response200
    
def removeNode( node_id ):
    return response200

def stopNode( node_id ):
    return response200

def startNode( node_id ):
    return response200

def startService(node_id, service_id):
    return response200

def stopService(node_id, service_id):
    return response200

def removeService(node_id, service_id):
    return response200

def addService(node_id, service):
    return response200

def shutdownDomain( domain_name ):
    return responseOk

def startDomain( domain_name ):
    return responseOk

def listLocalDomains():
    
    domain = Domain()
    domains = []
    domains.append(domain)
    
    return domains

def findDomainById(an_id):
    domain = Domain()
    domain.id = an_id
    domain.name = "MyDomain"
    domain.site = "MySite"
    
    return domain

def createSnapshotPolicy( volume_id, policy ):
    policy.id = 100
    return policy

def editSnapshotPolicy( policy ):
    return policy

def listSnapshotPolicies( volume_id=None ):
    policies = []
    policy = SnapshotPolicy()
    policy.id = 900
    policies.append( policy )
    return policies

def attachPolicy( policy_id, volume_id ):
    return responseOk

def detachPolicy( policy_id, volume_id ):
    return responseOk

def deleteSnapshotPolicy( volume_id, policy_id ):
    return responseOk 


def listTimelinePresets(preset_id=None):
    p = DataProtectionPolicyPreset()
    p.id = 1
    p.policies = [SnapshotPolicy()]
    presets = [p]
    return presets

def listQosPresets(preset_id=None):
    p = QosPreset()
    p.id = 1
    p.priority = 1
    p.iops_guarantee = 1
    p.iops_limit = 1
    presets = [p]
    return presets

def listUsers(tenant_id=1):
    user = User()
    user.username = "jdoe"
    user.id = 23
    
    return [user]

def createUser(user):
    return user

def listTenants():
    tenant = Tenant()
    tenant.name = "coolness"
    tenant.id = 1
    tenant.users = listUsers()
    return [tenant]

def createTenant(tenant):
    tenant.id = 2
    return tenant

def assignUser(tenant_id, user_id):
    return responseOk

def removeUser(tenant_id, user_id):
    return responseOk

def changePassword(user_id, password):
    return responseOk

def reissueToken(user_id):
    return responseOk

def get_token(user_id):
    d = dict()
    d["token"] = "totallyfaketoken"
    return d

def whoami():
    user = User()
    user.username = "me"
    user.id = 100
    return user

def getSystemHealth():
    health = SystemHealth()
    cap = HealthRecord(state=HealthState.BAD,category=HealthCategory.CAPACITY,message="l_capacity_bad_rate")
    serv = HealthRecord(state=HealthState.GOOD,category=HealthCategory.SERVICES,message="l_services_good")
    fb = HealthRecord(state=HealthState.GOOD,category=HealthCategory.FIREBREAK,message="l_firebreak_good")
    
    health.health_records.append( cap )
    health.health_records.append( serv )
    health.health_records.append( fb )
    
    health.overall_health = HealthState.MARGINAL
    
    return health

def fakeStats( query ):
    
    stats = Statistics()
    series = Series()
    
    series.context = Volume( an_id=1,name="TestVol" )
    series.type = "GETS"
    series.datapoints.append( Datapoint(x=0, y=1) )
    series.datapoints.append( Datapoint(x=20, y=100) )
    
    stats.series_list.append( series )
    
    stats.calculated_values.append( Calculated(key="total", value=3000 ) )
    
    return stats

def mockPostNode( session, url, data=None, callback=None, callback2=None ):
    nodes = listNodes()
    
    if len(nodes) > 0:
        return NodeConverter.to_json(nodes[0])
    
    return ""
