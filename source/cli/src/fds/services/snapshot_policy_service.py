from abstract_service import AbstractService
from fds.utils.snapshot_policy_converter import SnapshotPolicyConverter

class SnapshotPolicyService( AbstractService ):
    '''
    Created on Apr 23, 2015
    
    @author: nate
    '''
    
    def __init__(self, session):
        AbstractService.__init__(self, session)
        
    def create_snapshot_policy(self, policy):
        '''
        Create the snapshot policy
        '''
        url = "{}{}".format( self.get_url_preamble(), "/api/config/snapshot/policies" )
        data = SnapshotPolicyConverter.to_json( policy )
        return self.rest_helper().post( self.session, url, data )

    def delete_snapshot_policy(self, policy_id):
        '''
        delete the specified snapshot policy
        '''
        
        url = "{}{}{}".format( self.get_url_preamble(), "/api/config/snapshot/policies/", policy_id)
        return self.rest_helper().delete( self.session, url )
    
    def edit_snapshot_policy(self, policy ):
        '''
        edit a specified policy with new settings
        '''
        
        url = "{}{}".format( self.get_url_preamble(), "/api/config/snapshot/policies" )
        data = SnapshotPolicyConverter.to_json(policy)
        return self.rest_helper().put( self.session, url, data )
    
    def attach_snapshot_policy(self, policy_id, volume_id ):
        '''
        attach the given policy to the specified volume
        '''
        
        url = "{}{}{}{}{}".format( self.get_url_preamble(), "/api/config/snapshot/policies/", policy_id, "/attach", volume_id )
        return self.rest_helper().put( self.session, url )
    
    def detach_snapshot_policy(self, policy_id, volume_id):
        '''
        detach the given policy from the specified volume
        '''
        
        url = "".format( self.get_url_preamble(), "/api/config/snapshot/policies/", policy_id, "/detach", volume_id )
        return self.rest_helper().put( self.session, url )
    
    def list_snapshot_policies(self):
        '''
        Get a list of all the snapshot policies that exist in the system
        '''
        
        url = "{}{}".format( self.get_url_preamble(), "/api/config/snapshot/policies")
        return self.rest_helper().get( self.session, url )
    
    def list_snapshot_policies_by_volume(self, volume_id):
        '''
        Get a list of all snapshot policies that are attached to the specified volume
        '''
        
        url = "".format( self.get_url_preamble(), "/api/config/volumes/", volume_id, "/snapshot/policies" )
        return self.rest_helper().get( self.session, url )
        