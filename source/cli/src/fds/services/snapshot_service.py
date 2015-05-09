from abstract_service import AbstractService
from fds.utils.snapshot_converter import SnapshotConverter

class SnapshotService( AbstractService ):

    '''
    Created on Apr 22, 2015
    
    This class wraps all of the REST endpoints associated with snapshots.  It "speaks" the snapshot model object
    and JSON and abstracts away all of the web protocols as a sort of snapshot SDK for FDS
    
    @author: nate
    '''
    
    def __init__(self, session):
        AbstractService.__init__(self, session)

    def get_snapshot_by_id(self, an_id):
        '''
        retrieve a specific snapshot using its ID
        '''
        
        url = "{}{}{}".format( self.get_url_preamble(), "/api/config/snapshots/", an_id )
        response = self.rest_helper.get( self.session, url )
        
        snapshot = SnapshotConverter.build_snapshot_from_json( response )
        
        return snapshot        