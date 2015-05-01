from abstract_service import AbstractService
from fds.utils.volume_converter import VolumeConverter
from fds.utils.snapshot_converter import SnapshotConverter
from fds.services.snapshot_service import SnapshotService


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
            
    def find_volume_from_snapshot_id(self, snapshotId ):
        '''
        try to find the volume from the snapshot ID
        '''
        snapshot_service = SnapshotService( self.__session)
        snapshot = snapshot_service.get_snapshot_by_id( snapshotId )
        
        if ( snapshot == None ):
            return
        
        volume = self.find_volume_by_id( snapshot.volume_id )
        return volume
    
    def list_volumes(self):
        
        '''
        Return the raw json list of volumes from the FDS REST call
        '''
        url = "{}{}".format( self.get_url_preamble(), "/api/config/volumes" )
        response = self.rest_helper.get( self.session, url )
        
        volumes = []
        
        for j_volume in response:
            volume = VolumeConverter.build_volume_from_json( j_volume )
            volumes.append( volume )
            
        return volumes
    
    def create_volume(self, volume):
        '''
        Takes the passed in volume, converts it to JSON and uses the FDS REST
        endpoint to make the creation request
        '''
        
        url = "{}{}".format( self.get_url_preamble(), "/api/config/volumes" )
        data = VolumeConverter.to_json( volume )
        return self.rest_helper.post( self.session, url, data )
    
    def clone_from_snapshot_id(self, snapshot_id, volume):
        '''
        Use a snapshot ID and volume QoS settings to clone a new volume
        '''
        
        url = "{}{}{}/{}".format( self.get_url_preamble(), "/api/config/snapshot/clone/", snapshot_id, volume.name )
        data = VolumeConverter.to_json( volume )
        return self.rest_helper.post( self.session, url, data )
    
    def clone_from_timeline(self, a_time, volume ):
        '''
        Create a clone of the specified volume from the closest snapshot to the time provided
        '''
        
        url = "{}{}{}/{}/{}".format( self.get_url_preamble(), "/api/config/volumes/clone/", volume.id, volume.name, a_time )
        data = VolumeConverter.to_json( volume )
        return self.rest_helper.post( self.session, url, data )
    
    def edit_volume(self, volume):
        '''
        Takes a volume formatted object and tries to make the edits
        to the volume it points to
        '''
        
        url = "{}{}{}".format( self.get_url_preamble(), "/api/config/volumes/", str(volume.id) )
        data = VolumeConverter.to_json( volume )
        return self.rest_helper.put( self.session, url, data )
    
    def delete_volume(self, name):
        '''
        Deletes a volume based on the volume name.  It expects this name to be unique
        '''
        
        url = "{}{}{}".format( self.get_url_preamble(), "/api/config/volumes/", name )
        return self.rest_helper.delete( self.session, url )
    
    def create_snapshot(self, snapshot ):
        '''
        Create a snapshot for the volume specified
        '''
        
        url = "{}{}{}{}".format( self.get_url_preamble(), "/api/config/volumes/", snapshot.volume_id, "/snapshot" )
        data = SnapshotConverter.to_json(snapshot)
        return self.rest_helper.post( self.session, url, data )
    
    def list_snapshots(self, an_id):
        '''
        Get a list of all the snapshots that exists for a given volume
        '''
        
        url = "{}{}{}{}".format ( self.get_url_preamble(), "/api/config/volumes/", an_id, "/snapshots" )
        response = self.rest_helper.get( self.session, url )
        
        snapshots = []
        
        for j_snapshot in response:
            snapshot = SnapshotConverter.build_snapshot_from_json( j_snapshot )
            snapshots.append( snapshot )
            
        return snapshots
    
    def list_volume_ids_by_snapshot_policy(self, snapshot_policy_id ):
        '''
        Get a list of all volume IDs associated with this snapshot policy ID
        '''
        
        url = "{}{}{}{}".format( self.get_url_preamble(), "/api/config/snapshots/policies/", snapshot_policy_id, "/volumes")
        return self.rest_helper().get( self.session, url )    
