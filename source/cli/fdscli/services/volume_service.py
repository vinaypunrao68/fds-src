from .abstract_service import AbstractService
from utils.converters.volume.volume_converter import VolumeConverter
from utils.converters.volume.snapshot_converter import SnapshotConverter
from utils.converters.volume.preset_converter import PresetConverter
from model.fds_error import FdsError

class VolumeService( AbstractService ):

    '''
    Created on Apr 3, 2015
    
    This class wraps all of the REST endpoints associated with volumes.  It "speaks" the volume model object
    and JSON and abstracts away all of the web protocols as a sort of volume SDK for FDS
    
    @author: nate
    '''
    
    def __init__(self, session):
        AbstractService.__init__(self, session)
            
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
        
        if isinstance(response, FdsError):
            return response
        
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
        
        if isinstance(response, FdsError):
            return response
        
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

        if isinstance(j_volume, FdsError):
            return j_volume

        volume = VolumeConverter.build_volume_from_json( j_volume )
        return volume
    
    def clone_from_snapshot_id(self, volume, snapshot_id):
        ''' 
        clone from the given volume and snapshot ID
        '''
        url = "{}{}{}{}{}".format(self.get_url_preamble(), "/volumes/", volume.id, "/snapshot/", snapshot_id)
        data = VolumeConverter.to_json(volume)
        newVolume = self.rest_helper.post(self.session, url, data );
        
        if isinstance(newVolume, FdsError):
            return newVolume
        
        newVolume = VolumeConverter.build_volume_from_json(newVolume);
        return newVolume;
    
    def clone_from_timeline(self, volume, fromTime):
        '''
        Use a time and volume QoS settings to clone a new volume
        '''
        
        url = "{}{}{}{}{}".format( self.get_url_preamble(), "/volumes/", volume.id, "/time/", fromTime )
        data = VolumeConverter.to_json( volume )
        volume = self.rest_helper.post( self.session, url, data )

        if isinstance(volume, FdsError):
            return volume

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
        
        if isinstance(j_volume, FdsError):
            return j_volume
        
        volume = VolumeConverter.build_volume_from_json(j_volume)
        return volume
    
    def delete_volume(self, volume_id):
        '''
        Deletes a volume based on the volume name.  It expects this name to be unique
        '''
        
        url = "{}{}{}".format( self.get_url_preamble(), "/volumes/", volume_id )
        response = self.rest_helper.delete( self.session, url )
        
        if isinstance(response, FdsError):
            return response
        
        return response
    
    def create_snapshot(self, snapshot ):
        '''
        Create a snapshot for the volume specified
        '''
        
        url = "{}{}{}{}".format( self.get_url_preamble(), "/volumes/", snapshot.volume_id, "/snapshots" )
        data = SnapshotConverter.to_json(snapshot)
        response = self.rest_helper.post( self.session, url, data )
        
        if isinstance(response, FdsError):
            return response
        
        return response
    
    def delete_snapshot(self, volume_id, snapshot_id):
        '''
        Delete a specific snapshot from a volume
        '''
        
        url = "{}{}{}{}{}".format( self.get_url_preamble(), "/volumes", volume_id, "/snapshot", snapshot_id )
        response = self.rest_helper.delete( url )
        
        if isinstance(response, FdsError):
            return response
        
        return response
    
    def list_snapshots(self, volume_id):
        '''
        Get a list of all the snapshots that exists for a given volume
        '''
        
        url = "{}{}{}{}".format ( self.get_url_preamble(), "/volumes/", volume_id, "/snapshots" )
        response = self.rest_helper.get( self.session, url )
        
        if isinstance(response, FdsError):
            return response
    
        snapshots = []
        
        for j_snapshot in response:
            snapshot = SnapshotConverter.build_snapshot_from_json( j_snapshot )
            snapshots.append( snapshot )
            
        return snapshots
    
    def get_data_protection_presets(self, preset_id=None):
        '''
        Get a list of timeline preset policies
        '''
        
        url = "{}{}".format( self.get_url_preamble(), "/presets/data_protection_policies")
        response = self.rest_helper.get( self.session, url )
        
        if isinstance(response, FdsError):
            return response
        
        presets = []
        
        for j_preset in response:
            
            if preset_id != None and int(j_preset["id"]) != int(preset_id):
                continue
            
            preset = PresetConverter.build_timeline_from_json( j_preset )
            presets.append( preset )
            
        return presets
    
    def get_qos_presets(self, preset_id=None):
        '''
        Get a list of QoS preset policies
        '''
        
        url = "{}{}".format( self.get_url_preamble(), "/presets/quality_of_service_policies" )
        response = self.rest_helper.get( self.session, url )
        
        if isinstance(response, FdsError):
            return response   
        
        presets = []
        
        for j_preset in response:
            
            if preset_id != None and int(j_preset["id"]) != int(preset_id):
                continue
            
            preset = PresetConverter.build_qos_preset_from_json( j_preset )
            presets.append( preset )
            
        return presets  
