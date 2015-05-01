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

'''
Created on Apr 22, 2015

@author: nate
'''

def listVolumes():
    volume = Volume()
    vols = []
    tempStr = VolumeConverter.to_json( volume )
    volume = json.loads( tempStr )
    vols.append( volume )
    
    return vols

def createVolume(volume):
    volume = VolumeConverter.to_json( volume )
    return volume

def editVolume(volume):
    volume = VolumeConverter.to_json( volume )
    return volume

def cloneFromTimelineTime( a_time, volume ):
    
    volume.id = 354
    volume.timeline_time = a_time
    volume = VolumeConverter.to_json(volume)
    return volume

def cloneFromSnapshotId( snapshot_id, volume):
    volume.id = 789
    volume = VolumeConverter.to_json( volume )
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
    tempSnap = SnapshotConverter.to_json( snapshot )
    snaps = []
    snapshot = json.loads( tempSnap )
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
    
def deactivateNode( node_id, state ):
    response = dict()
    response["status"] = "OK"
    return response   

def shutdownDomain( domain_name ):
    response = dict()
    response["status"] = "OK"
    return response 

def listLocalDomains():
    
    domain = Domain()
    domain = DomainConverter.to_json(domain)
    domains = []
    domains.append( json.loads( domain ))
    
    return domains

def createSnapshotPolicy( policy ):
    policy.id = 100
    return policy

def editSnapshotPolicy( policy ):
    return policy

def listSnapshotPolicies( volume_id=None ):
    policies = []
    policy = SnapshotPolicy()
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