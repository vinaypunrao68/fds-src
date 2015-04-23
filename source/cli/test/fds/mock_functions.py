from fds.utils.volume_converter import VolumeConverter
from fds.model.volume import Volume
from fds.model.snapshot import Snapshot
import json
from fds.utils.snapshot_converter import SnapshotConverter

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
    