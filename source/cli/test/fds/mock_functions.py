from fds.utils.volume_converter import VolumeConverter
from fds.model.volume import Volume
from fds.model.snapshot import Snapshot
from fds.model.node import Node
from fds.model.service import Service
from fds.model.domain import Domain
import json
from fds.utils.snapshot_converter import SnapshotConverter
from fds.utils.node_converter import NodeConverter
from fds.utils.domain_converter import DomainConverter
from fds.model.snapshot_policy import SnapshotPolicy
from fds.model.timeline_preset import TimelinePreset
from fds.model.qos_preset import QosPreset

'''
Created on Apr 22, 2015

@author: nate
'''

def listVolumes():
    volume = Volume()
    vols = []

    vols.append( volume )
    
    return vols

def createVolume(volume):
    return volume

def editVolume(volume):
    return volume

def cloneFromTimelineTime( a_time, volume ):
    
    volume.id = 354
    volume.timeline_time = a_time
    return volume

def cloneFromSnapshotId( snapshot_id, volume):
    volume.id = 789
    return volume

def deleteVolume(name):
    
    response = dict()
    response["status"] = "OK"
    return response

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
    response = dict()
    response["status"] = "OK"
    return response

def listSnapshots( volumeName ):
    snapshot = Snapshot()
    snaps = []
    snaps.append( snapshot )
    return snaps

def listNodes():
    node = Node()
    node.name = "FakeNode"
    node.ip_v4_address = "10.12.14.15"
    node.id = "21ABC"
    node.state = "ACTIVE"
    
    node.services["AM"]  = [Service(a_type="FDSP_ACCESS_MGR",auto_name="AM")]
    node.services["DM"]  = [Service(a_type="FDSP_DATA_MGR",auto_name="DM")]
    node.services["PM"]  = [Service(a_type="FDSP_PLATFORM",auto_name="PM")]
    node.services["SM"]  = [Service(a_type="FDSP_STOR_MGR",auto_name="SM")]                
 
    nodes = [node]
    return nodes

def activateNode( node_id, state ):
    response = dict()
    response["status"] = "OK"
    return response
    
def deactivateNode( node_id ):
    response = dict()
    response["status"] = "OK"
    return response   

def shutdownDomain( domain_name ):
    response = dict()
    response["status"] = "OK"
    return response 

def startDomain( domain_name ):
    response = dict()
    response["status"] = "OK"
    return response

def listLocalDomains():
    
    domain = Domain()
    domains = []
    domains.append(domain)
    
    return domains

def createSnapshotPolicy( policy ):
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
    response = dict()
    response["status"] = "OK"
    return response 

def detachPolicy( policy_id, volume_id ):
    response = dict()
    response["status"] = "OK"
    return response 

def deleteSnapshotPolicy( policy_id ):
    response = dict()
    response["status"] = "OK"
    return response 

def listTimelinePresets(preset_id=None):
    p = TimelinePreset()
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