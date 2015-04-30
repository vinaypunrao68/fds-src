from base_cli_test import BaseCliTest
import mock_functions
from mock import patch

class TestSnapshotPolicyCreate(BaseCliTest):
    '''
    Created on Apr 30, 2015
    
    @author: nate
    '''
    
    @patch( "fds.services.snapshot_policy_service.SnapshotPolicyService.list_snapshot_policies", side_effect=mock_functions.listSnapshotPolicies)
    @patch( "fds.services.snapshot_policy_service.SnapshotPolicyService.create_snapshot_policy", side_effect=mock_functions.createSnapshotPolicy)
    def test_policy_create_defaults(self, mockCreate, mockList):
        '''
        Test that a snapshot policy created with the minimal set of data fills in the correct defaults
        '''
        
        args = ["snapshot_policy", "create", "-name=MyPolicy"]
        
        self.callMessageFormatter(args)
        self.cli.run( args )
        
        assert mockCreate.call_count == 1
        assert mockList.call_count == 1
        
        policy = mockCreate.call_args[0][0]
        
        assert policy.name == "MyPolicy"
        assert policy.retention == 0
        assert policy.recurrence_rule.frequency == "DAILY"
        assert policy.recurrence_rule.byhour == [0]
        assert policy.recurrence_rule.byminute == [0]
        
    @patch( "fds.services.snapshot_policy_service.SnapshotPolicyService.list_snapshot_policies", side_effect=mock_functions.listSnapshotPolicies)
    @patch( "fds.services.snapshot_policy_service.SnapshotPolicyService.create_snapshot_policy", side_effect=mock_functions.createSnapshotPolicy)
    def test_policy_create(self, mockCreate, mockList):
        '''
        Test policy creation with command line arguments
        '''
        
        args = ["snapshot_policy", "create", "-name=MyPolicy", "-frequency=WEEKLY", "-hour", "3", "12", "-day_of_week", "SU", "WE", "-minute", "15", "30", "45"]
        
        self.callMessageFormatter(args)
        self.cli.run(args)        
        
        assert mockCreate.call_count == 1
        assert mockList.call_count == 1
        
        policy = mockCreate.call_args[0][0]
        
        assert policy.name == "MyPolicy"
        assert policy.recurrence_rule.frequency == "WEEKLY"
        assert policy.recurrence_rule.byhour == [3, 12]
        assert policy.recurrence_rule.byminute == [15,30,45]
        assert policy.recurrence_rule.byday == ["SU", "WE" ]
        
    @patch( "fds.services.snapshot_policy_service.SnapshotPolicyService.list_snapshot_policies", side_effect=mock_functions.listSnapshotPolicies)
    @patch( "fds.services.snapshot_policy_service.SnapshotPolicyService.create_snapshot_policy", side_effect=mock_functions.createSnapshotPolicy)
    def test_policy_create_restrictions(self, mockCreate, mockList):  
        '''
        test policy creation and check the restrictions are being enforced
        '''
        #no name
        args = [ "snapshot_policy", "create", "-frequency=DAILY", "-day_of_week", "SU", "MO" ]
        
        self.callMessageFormatter(args)
        self.cli.run( args )
        
        assert mockCreate.call_count == 0
        assert mockList.call_count == 0
        
        # make sure days don't make it in if its daily
        args.append( "-name=MyPolicy" )
        self.callMessageFormatter(args)
        self.cli.run(args)
        
        assert mockCreate.call_count == 1
        assert mockList.call_count == 1
        
        policy = mockCreate.call_args[0][0]
        
        assert policy.recurrence_rule.byday == None
        
        #make sure month, day of month and day of year don't make it in a weekly
        args = ["snapshot_policy", "create", "-name=MyPolicy", "-frequency=WEEKLY", "-day_of_month", "15", "-day_of_year", "255", "-month", "2", "5", "-day_of_week", "TH"]
        
        self.callMessageFormatter(args)
        self.cli.run(args)

        assert mockCreate.call_count == 2
        assert mockList.call_count == 2
        
        policy = mockCreate.call_args[0][0]
        
        assert policy.recurrence_rule.bymonth == None
        assert policy.recurrence_rule.byyearday == None
        assert policy.recurrence_rule.bymonthday == None
        assert policy.recurrence_rule.byday == ["TH"]
        
        #make sure month and day of the year dont' make it in the monthly
        args[3] = "-frequency=MONTHLY"
        
        self.callMessageFormatter(args)
        self.cli.run(args)
        
        assert mockCreate.call_count == 3
        assert mockList.call_count == 3
        
        policy = mockCreate.call_args[0][0]
        
        assert policy.recurrence_rule.bymonth == None
        assert policy.recurrence_rule.byyearday == None
        assert policy.recurrence_rule.bymonthday == [15]
        assert policy.recurrence_rule.byday == ["TH"]
        
        #make sure everything gets in if its yearly
        args[3] = "-frequency=YEARLY"
        
        self.callMessageFormatter(args)
        self.cli.run(args)
        
        assert mockCreate.call_count == 4
        assert mockList.call_count == 4
        
        policy = mockCreate.call_args[0][0]
        
        assert policy.recurrence_rule.bymonth == [2,5]
        assert policy.recurrence_rule.byyearday == [255]
        assert policy.recurrence_rule.bymonthday == [15]
        assert policy.recurrence_rule.byday == ["TH"]  
        
    @patch( "fds.services.snapshot_policy_service.SnapshotPolicyService.list_snapshot_policies", side_effect=mock_functions.listSnapshotPolicies)
    @patch( "fds.services.snapshot_policy_service.SnapshotPolicyService.create_snapshot_policy", side_effect=mock_functions.createSnapshotPolicy)
    def test_policy_create_data(self, mockCreate, mockList):
        '''
        Try to create a snapshot policy using a JSON string instead of arguments
        '''
        
        jString = "{\"name\":\"MyPolicy\",\"retention\":86400,\"timelineTime\":0,\"recurrenceRule\":{\"FREQ\":\"WEEKLY\",\"BYDAY\":[\"SU\"]}}"
        
        args = ["snapshot_policy", "create", "-data=" + jString]
        self.callMessageFormatter(args)
        self.cli.run(args)
        
        assert mockCreate.call_count == 1
        assert mockList.call_count == 1
        
        policy = mockCreate.call_args[0][0]
        
        assert policy.name == "MyPolicy"
        assert policy.recurrence_rule.frequency == "WEEKLY"
        assert policy.retention == 86400
        assert policy.timeline_time == 0
        assert policy.recurrence_rule.byday == ["SU"]        
        