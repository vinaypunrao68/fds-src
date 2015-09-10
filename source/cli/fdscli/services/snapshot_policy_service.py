from .abstract_service import AbstractService

from utils.converters.volume.snapshot_policy_converter import SnapshotPolicyConverter
from model.fds_error import FdsError

class SnapshotPolicyService( AbstractService ):
    '''
    Created on Apr 23, 2015
    
    @author: nate
    '''
    
    def __init__(self, session):
        AbstractService.__init__(self, session)
        
    def create_snapshot_policy(self, volume_id, policy):
        '''
        Create the snapshot policy
        
        policy is the SnapshotPolicy object that defines the policy to create
        '''
        url = "{}{}{}{}".format( self.get_url_preamble(), "/volumes/", volume_id, "/snapshot_policies" )
        data = SnapshotPolicyConverter.to_json( policy )
        j_policy =  self.rest_helper.post( self.session, url, data )
        
        if isinstance(j_policy, FdsError):
            return j_policy
        
        policy = SnapshotPolicyConverter.build_snapshot_policy_from_json( j_policy )
        return policy

    def delete_snapshot_policy(self, volume_id, policy_id):
        '''
        delete the specified snapshot policy
        
        policy_id is the UUID of the policy that you would like to delete
        '''
        
        url = "{}{}{}{}{}".format( self.get_url_preamble(), "/volumes/", volume_id, "/snapshot_policies/", policy_id)
        response = self.rest_helper.delete( self.session, url )
        
        if isinstance(response, FdsError):
            return response
        
        return response
    
    def edit_snapshot_policy(self, volume_id, policy ):
        '''
        edit a specified policy with new settings
        
        policy is the SnapshotPolicy object that holds the new settings.  It will replace the policy by matching the UUID in the object with the one in the system
        '''
        
        url = "{}{}{}{}{}".format( self.get_url_preamble(), "/volumes/", volume_id, "/snapshot_policies/", policy.id.uuid )
        data = SnapshotPolicyConverter.to_json(policy)
        j_policy = self.rest_helper.put( self.session, url, data )
        
        if isinstance(j_policy, FdsError):
            return j_policy
        
        policy = SnapshotPolicyConverter.build_snapshot_policy_from_json(j_policy)
        return policy
    
    def list_snapshot_policies(self, volume_id):
        '''
        Get a list of all the snapshot policies that exist in the system
        '''
        
        url = "{}{}{}{}".format( self.get_url_preamble(), "/volumes/", volume_id, "/snapshot_policies")
        j_policies = self.rest_helper.get( self.session, url )
        
        if isinstance(j_policies, FdsError):
            return j_policies    
        
        policies = []
        
        for j_policy in j_policies:
            policy = SnapshotPolicyConverter.build_snapshot_policy_from_json( j_policy )
            policies.append( policy )
            
        return policies
        
