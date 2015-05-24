from abstract_service import AbstractService
from ..utils.snapshot_policy_converter import SnapshotPolicyConverter

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
        
        policy is the SnapshotPolicy object that defines the policy to create
        '''
        url = "{}{}".format( self.get_url_preamble(), "/api/config/snapshot/policies" )
        data = SnapshotPolicyConverter.to_json( policy )
        j_policy =  self.rest_helper.post( self.session, url, data )
        policy = SnapshotPolicyConverter.build_snapshot_policy_from_json( j_policy )
        return policy

    def delete_snapshot_policy(self, policy_id):
        '''
        delete the specified snapshot policy
        
        policy_id is the UUID of the policy that you would like to delete
        '''
        
        url = "{}{}{}".format( self.get_url_preamble(), "/api/config/snapshot/policies/", policy_id)
        return self.rest_helper.delete( self.session, url )
    
    def edit_snapshot_policy(self, policy ):
        '''
        edit a specified policy with new settings
        
        policy is the SnapshotPolicy object that holds the new settings.  It will replace the policy by matching the UUID in the object with the one in the system
        '''
        
        url = "{}{}".format( self.get_url_preamble(), "/api/config/snapshot/policies" )
        data = SnapshotPolicyConverter.to_json(policy)
        return self.rest_helper.put( self.session, url, data )
    
    def attach_snapshot_policy(self, policy_id, volume_id ):
        '''
        attach the given policy to the specified volume
        
        policy_id is the UUID of the policy you'd like to attach
        volume_id is the UUID of the volume you are attaching the policy to
        '''
        
        url = "{}{}{}{}{}".format( self.get_url_preamble(), "/api/config/snapshot/policies/", policy_id, "/attach/", volume_id )
        return self.rest_helper.put( self.session, url )
    
    def detach_snapshot_policy(self, policy_id, volume_id):
        '''
        detach the given policy from the specified volume
        
        policy_id is the UUID of the policy you'd like to detach
        volume_id is the UUID of the volume you are detaching the policy from
        '''
        
        url = "{}{}{}{}{}".format( self.get_url_preamble(), "/api/config/snapshot/policies/", policy_id, "/detach/", volume_id )
        return self.rest_helper.put( self.session, url )
    
    def list_snapshot_policies(self):
        '''
        Get a list of all the snapshot policies that exist in the system
        '''
        
        url = "{}{}".format( self.get_url_preamble(), "/api/config/snapshot/policies")
        j_policies = self.rest_helper.get( self.session, url )
        
        policies = []
        
        for j_policy in j_policies:
            policy = SnapshotPolicyConverter.build_snapshot_policy_from_json( j_policy )
            policies.append( policy )
            
        return policies
    
    def list_snapshot_policies_by_volume(self, volume_id):
        '''
        Get a list of all snapshot policies that are attached to the specified volume
        
        volume_id is the UUID for the volume you would like to see the attached policies for
        '''
        
        url = "{}{}{}{}".format( self.get_url_preamble(), "/api/config/volumes/", volume_id, "/snapshot/policies" )
        j_policies = self.rest_helper.get( self.session, url )
        
        policies = []
        
        for j_policy in j_policies:
            policy = SnapshotPolicyConverter.build_snapshot_policy_from_json( j_policy )
            policies.append( policy )
            
        return policies
        
