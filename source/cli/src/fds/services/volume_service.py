from abstract_service import AbstractService

from fds.utils.converters.volume.volume_converter import VolumeConverter
from fds.utils.converters.volume.snapshot_converter import SnapshotConverter
from fds.utils.converters.volume.preset_converter import PresetConverter

class VolumeService( AbstractService ):

    '''
    Created on Apr 3, 2015
    
    This class wraps all of the REST endpoints associated with volumes.  It "speaks" the volume model object
    and JSON and abstracts away all of the web protocols as a sort of volume SDK for FDS
    
    @author: nate
    '''
    
    def __init__(self, session):
        AbstractService.__init__(self, session)

    
    def find_volume_by_id(self, an_id):
        '''
        Get a volume by its UUID.  Hopefully this is temporary code only while the REST services
        don't support taking either the ID or the name.  Currently they only take a name.
        '''
        volumes = self.list_volumes()
        
        for volume in volumes:
            if ( volume.id == an_id ):
                return volume
            
    def find_volume_by_name(self, aName):  
        '''
        Get a volume by its name.  Hopefully this is temporary code only while the REST services
        don't support taking either the ID or the name.  Currently they only take a name.
        '''
        volumes = self.list_volumes()
        
        for volume in volumes:
            if ( volume.name == aName ):
                return volume  
    
    def list_volumes(self):
        
        '''
        Return the raw json list of volumes from the FDS REST call
        '''
        url = "{}{}".format( self.get_url_preamble(), "/volumes" )
        response = self.rest_helper.get( self.session, url )
        
        volumes = []
        
        for j_volume in response:
            volume = VolumeConverter.build_volume_from_json( j_volume )
            volumes.append( volume )
            
        return volumes
    
    def get_volume(self, volume_id):
        ''' 
        get a single volume
        '''
        
        url = "{}{}{}".format( self.get_url_preamble(), "/volumes/", volume_id)
        response = self.rest_helper.get(self.session, url)
        
        volume = VolumeConverter.build_volume_from_json(response)
        
        return volume
    
    def create_volume(self, volume):
        '''
        Takes the passed in volume, converts it to JSON and uses the FDS REST
        endpoint to make the creation request
        '''
        
        url = "{}{}".format( self.get_url_preamble(), "/volumes" )
        data = VolumeConverter.to_json( volume )
        j_volume = self.rest_helper.post( self.session, url, data )
        
        volume = VolumeConverter.build_volume_from_json( j_volume )
        return volume
    
    def clone_to_volume(self, volume_id, volume):
        '''
        Use a snapshot ID and volume QoS settings to clone a new volume
        '''
        
        url = "{}{}{}/{}".format( self.get_url_preamble(), "/volumes/", volume_id )
        data = VolumeConverter.to_json( volume )
        volume = self.rest_helper.post( self.session, url, data )
        volume = VolumeConverter.build_volume_from_json( volume )
        return volume
    
    def edit_volume(self, volume):
        '''
        Takes a volume formatted object and tries to make the edits
        to the volume it points to
        '''
        
        url = "{}{}{}".format( self.get_url_preamble(), "/volumes/", str(volume.id) )
        data = VolumeConverter.to_json( volume )
        j_volume = self.rest_helper.put( self.session, url, data )
        
        volume = VolumeConverter.build_volume_from_json(j_volume)
        return volume
    
    def delete_volume(self, volume_id):
        '''
        Deletes a volume based on the volume name.  It expects this name to be unique
        '''
        
        url = "{}{}{}".format( self.get_url_preamble(), "/volumes/", volume_id )
        return self.rest_helper.delete( self.session, url )
    
    def create_snapshot(self, volume_id ):
        '''
        Create a snapshot for the volume specified
        '''
        
        url = "{}{}{}{}".format( self.get_url_preamble(), "/volumes", volume_id, "/snapshot" )
        return self.rest_helper.post( self.session, url )
    
    def delete_snapshot(self, volume_id, snapshot_id):
        '''
        Delete a specific snapshot from a volume
        '''
        
        url = "{}{}{}".format( self.get_url_preamble(), "/volumes", volume_id, "/snapshot", snapshot_id )
        return self.rest_helper.delete( url )
    
    def list_snapshots(self, volume_id):
        '''
        Get a list of all the snapshots that exists for a given volume
        '''
        
        url = "{}{}{}{}".format ( self.get_url_preamble(), "/volumes/", volume_id, "/snapshots" )
        response = self.rest_helper.get( self.session, url )
        
        snapshots = []
        
        for j_snapshot in response:
            snapshot = SnapshotConverter.build_snapshot_from_json( j_snapshot )
            snapshots.append( snapshot )
            
        return snapshots
    
    def get_timeline_presets(self, preset_id=None):
        '''
        Get a list of timeline preset policies
        '''
        
        url = "{}{}".format( self.get_url_preamble(), "/api/config/volumes/presets/timeline")
        response = self.rest_helper.get( self.session, url )
        
        presets = []
        
        for j_preset in response:
            
            if preset_id != None and int(j_preset["uuid"]) != int(preset_id):
                continue
            
            preset = PresetConverter.build_timeline_from_json( j_preset )
            presets.append( preset )
            
        return presets
    
    def get_qos_presets(self, preset_id=None):
        '''
        Get a list of QoS preset policies
        '''
        
        url = "{}{}".format( self.get_url_preamble(), "/api/config/volumes/presets/qos" )
        response = self.rest_helper.get( self.session, url )
        
        presets = []
        
        for j_preset in response:
            
            if preset_id != None and int(j_preset["uuid"]) != int(preset_id):
                continue
            
            preset = PresetConverter.build_qos_preset_from_json( j_preset )
            presets.append( preset )
            
        return presets  
