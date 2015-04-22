from rest_helper import RESTHelper
from fds.utils.volume_converter import VolumeConverter
from fds.utils.snapshot_converter import SnapshotConverter


class VolumeService():

    '''
    Created on Apr 3, 2015
    
    This class wraps all of the REST endpoints associated with volumes.  It "speaks" the volume model object
    and JSON and abstracts away all of the web protocols as a sort of volume SDK for FDS
    
    @author: nate
    '''
    
    def __init__(self, session):
        self.__session = session
        self.__restHelper = RESTHelper()
        

    def __get_url_preamble(self):
        
        '''
        Helper method to construct the base URI
        '''        
        return "http://" + self.__session.get_hostname() + ":" + str( self.__session.get_port() )
    
    def find_volume_by_id(self, an_id):
        '''
        Get a volume by its UUID.  Hopefully this is temporary code only while the REST services
        don't support taking either the ID or the name.  Currently they only take a name.
        '''
        volumes = self.list_volumes()
        
        for volume in volumes:
            if ( volume["id"] == an_id ):
                return VolumeConverter.build_volume_from_json( volume )
            
    def find_volume_by_name(self, aName):  
        '''
        Get a volume by its name.  Hopefully this is temporary code only while the REST services
        don't support taking either the ID or the name.  Currently they only take a name.
        '''
        volumes = self.list_volumes()
        
        for volume in volumes:
            if ( volume["name"] == aName ):
                return VolumeConverter.build_volume_from_json( volume )  
            
    def find_volume_from_snapshot_id(self, snapshotId ):
        '''
        try to find the volume from the snapshot ID
        '''
        #TODO
        # need to get a snapshot object by ID - then use the volume ID
    
    def list_volumes(self):
        
        '''
        Return the raw json list of volumes from the FDS REST call
        '''
        url = self.__get_url_preamble() + "/api/config/volumes"
        return self.__restHelper.get( self.__session, url )
    
    def create_volume(self, volume):
        '''
        Takes the passed in volume, converts it to JSON and uses the FDS REST
        endpoint to make the creation request
        '''
        
        url = self.__get_url_preamble() + "/api/config/volumes"
        data = VolumeConverter.to_json( volume )
        return self.__restHelper.post( self.__session, url, data )
    
    def edit_volume(self, volume):
        '''
        Takes a volume formatted object and tries to make the edits
        to the volume it points to
        '''
        
        url = self.__get_url_preamble() + "/api/config/volumes/" + str(volume.id)
        data = VolumeConverter.to_json( volume )
        return self.__restHelper.put( self.__session, url, data )
    
    def delete_volume(self, name):
        '''
        Deletes a volume based on the volume name.  It expects this name to be unique
        '''
        
        url = self.__get_url_preamble() + "/api/config/volumes/" + name
        return self.__restHelper.delete( self.__session, url )
    
    def create_snapshot(self, snapshot ):
        '''
        Create a snapshot for the volume specified
        '''
        
        url = self.__get_url_preamble() + "/api/config/volumes/" + snapshot.volume_id + "/snapshot"
        data = SnapshotConverter.to_json(snapshot)
        return self.__restHelper.post( self.__session, url, data )
    
    def list_snapshots(self, name):
        '''
        Get a list of all the snapshots that exists for a given volume
        '''
        
        url = self.__get_url_preamble() + "/api/config/volumes/" + name + "/snapshots"
        return self.__restHelper.get( self.__session, url )
