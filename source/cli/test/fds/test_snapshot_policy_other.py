from base_cli_test import BaseCliTest
from mock import patch
import mock_functions

class TestSnapshotPolicyOther(BaseCliTest):
    '''
    Created on Apr 30, 2015
    
    @author: nate
    ''' 

    @patch( "fds.services.snapshot_policy_service.SnapshotPolicyService.list_snapshot_policies", side_effect=mock_functions.listSnapshotPolicies)
    @patch( "fds.services.snapshot_policy_service.SnapshotPolicyService.delete_snapshot_policy", side_effect=mock_functions.deleteSnapshotPolicy )
    def test_delete_snapshot_policy(self, mockDelete, mockList):            
        '''
        Test the deletion of a snapshot policy
        '''
        #no ID
        args = ["snapshot_policy", "delete"]
        
        self.callMessageFormatter(args)
        self.cli.run(args)
        
        assert mockDelete.call_count == 0
        assert mockList.call_count == 0
        
        args.append( "-policy_id=1" )

        self.callMessageFormatter(args)
        self.cli.run(args)
        
        assert mockDelete.call_count == 1
        assert mockList.call_count == 1
        
        policy_id = mockDelete.call_args[0][0]
        
        assert policy_id == "1"
        
        
