from abstract_plugin import AbstractPlugin
import json
from fds.model.snapshot_policy import SnapshotPolicy
from fds.model.recurrence_rule import RecurrenceRule
from fds.utils.snapshot_policy_converter import SnapshotPolicyConverter
from fds.utils.snapshot_policy_validator import SnapshotPolicyValidator
from fds.utils.recurrence_rule_converter import RecurrenceRuleConverter
from fds.services.snapshot_policy_service import SnapshotPolicyService
from fds.services.response_writer import ResponseWriter

'''
Created on Apr 13, 2015

A plugin to handle all the parsing for snapshot policy management
and to route those options to the appropriate snapshot policy 
management calls

@author: nate
'''
class SnapshotPolicyPlugin( AbstractPlugin):
    
    def __init__(self, session):
        AbstractPlugin.__init__(self, session) 
        self.__sp_service = SnapshotPolicyService( session )   
    
    '''
    @see: AbstractPlugin
    '''
    def build_parser(self, parentParser, session): 
        
        self.__parser = parentParser.add_parser( "snapshot_policy", help="Create, edit, delete and manipulate snapshot policies" )
        self.__subparser = self.__parser.add_subparsers( help="The sub-commands that are available")
        
        self.create_create_parser(self.__subparser)
        self.create_edit_parser(self.__subparser)
        self.create_list_parser(self.__subparser)
        self.create_delete_parser(self.__subparser)
        self.create_attach_parser(self.__subparser)
        self.create_detach_parser(self.__subparser)
     
    '''
    @see: AbstractPlugin
    '''   
    def detect_shortcut(self, args):
        
        return None
    
    def get_snapshot_policy_service(self):
        return self.__sp_service
    
    # builds the parsers
    def create_list_parser(self, subparser):
        ''' 
        parser for the list command
        '''
        
        __list_parser = subparser.add_parser( "list", help="List all of the snapshot policies in the system, or for a single volume.")
        self.add_format_arg( __list_parser )
        
        __list_parser.add_argument( "-" + AbstractPlugin.volume_id_str, help="If a volume UUID is specified it will only list the policies attached to the specified volume", default=None)
        
        __list_parser.set_defaults( func=self.list_snapshot_policies, format="tabular")
    
    def create_create_parser(self, subparser):
        '''
        Create a parser for the creation of a snapshot policy
        '''
        
        __create_parser = subparser.add_parser( "create", help="Create a new snapshot policy.")
        self.add_format_arg( __create_parser )
        
        __create_parser.add_argument( "-" + AbstractPlugin.data_str, help="A JSON formatted string that defines your policy entirely.  If this is present all other arguments will be ignored.", default=None )
        __create_parser.add_argument( "-" + AbstractPlugin.name_str, help="The name of the policy you are creating.", default=None)
        __create_parser.add_argument( "-" + AbstractPlugin.retention_str, help="The time (in seconds) that you want to keep snapshots that are created with this policy. 0 = forever", type=int, default=0)
        __create_parser.add_argument( "-" + AbstractPlugin.recurrence_rule_str, help="The iCal format recurrence rule you would like this policy to follow. http://www.kanzaki.com/docs/ical/rrule.html")
        __create_parser.add_argument( "-" + AbstractPlugin.frequency_str, help="The frequency for which you would like snapshots to be taken on volumes this policy is applied to.", choices=["DAILY", "HOURLY", "WEEKLY", "MONTHLY", "YEARLY"], default="DAILY")
        __create_parser.add_argument( "-" + AbstractPlugin.day_of_week_str, help="The day(s) of the week you would like the policy to run (when applicable).", choices=["SU","MO","TU","WE","TH","FR","SA"], nargs="+", default=["SU"])
        __create_parser.add_argument( "-" + AbstractPlugin.day_of_month_str, help="The number day you would like the policy to run.", nargs="+", type=SnapshotPolicyValidator.day_of_month_values)
        __create_parser.add_argument( "-" + AbstractPlugin.day_of_year_str, help="The number day you would like the policy to run.", nargs="+", type=SnapshotPolicyValidator.day_of_year_values)
        __create_parser.add_argument( "-" + AbstractPlugin.month_str, help="The month you would like this policy to run.", nargs="+", type=SnapshotPolicyValidator.month_value)
        __create_parser.add_argument( "-" + AbstractPlugin.hour_str, help="The hour(s) of the day you wish this policy to run.", nargs="+", type=SnapshotPolicyValidator.hour_value, default=[0])
        __create_parser.add_argument( "-" + AbstractPlugin.minute_str, help="The minute(s) of the day you wish this policy to run.", nargs="+", type=SnapshotPolicyValidator.minute_value, default=[0])
        
        __create_parser.set_defaults( func=self.create_snapshot_policy, format="tabular")
        
    def create_edit_parser(self, subparser):
        '''
        Create a parser for editing a snapshot policy
        '''
        
        __edit_parser = subparser.add_parser( "edit", help="Edit an existing snapshot policy")
        self.add_format_arg(__edit_parser)
        
        __edit_group = __edit_parser.add_mutually_exclusive_group(required=True)
        __edit_group.add_argument( "-" + AbstractPlugin.data_str, help="A JSON formatted string that defines your policy entirely.  If this is present all other arguments will be ignored.", default=None )
        __edit_group.add_argument( "-" + AbstractPlugin.policy_id_str, help="The UUID of the policy you would like to edit.", default=None )
        
        __edit_parser.add_argument( "-" + AbstractPlugin.name_str, help="The name of the policy you are creating.", default=None)
        __edit_parser.add_argument( "-" + AbstractPlugin.retention_str, help="The time (in seconds) that you want to keep snapshots that are created with this policy. 0 = forever", type=int, default=0)
        __edit_parser.add_argument( "-" + AbstractPlugin.recurrence_rule_str, help="The iCal format recurrence rule you would like this policy to follow. http://www.kanzaki.com/docs/ical/rrule.html")
        __edit_parser.add_argument( "-" + AbstractPlugin.frequency_str, help="The frequency for which you would like snapshots to be taken on volumes this policy is applied to.", choices=["DAILY", "HOURLY", "WEEKLY", "MONTHLY", "YEARLY"], default="DAILY")
        __edit_parser.add_argument( "-" + AbstractPlugin.day_of_week_str, help="The day(s) of the week you would like the policy to run (when applicable).", choices=["SU","MO","TU","WE","TH","FR","SA"], nargs="+", default=["SU"])
        __edit_parser.add_argument( "-" + AbstractPlugin.day_of_month_str, help="The number day you would like the policy to run.", nargs="+", type=SnapshotPolicyValidator.day_of_month_values)
        __edit_parser.add_argument( "-" + AbstractPlugin.day_of_year_str, help="The number day you would like the policy to run.", nargs="+", type=SnapshotPolicyValidator.day_of_year_values)
        __edit_parser.add_argument( "-" + AbstractPlugin.month_str, help="The month you would like this policy to run.", nargs="+", type=SnapshotPolicyValidator.month_value)
        __edit_parser.add_argument( "-" + AbstractPlugin.hour_str, help="The hour(s) of the day you wish this policy to run.", nargs="+", type=SnapshotPolicyValidator.hour_value, default=[0])
        __edit_parser.add_argument( "-" + AbstractPlugin.minute_str, help="The minute(s) of the day you wish this policy to run.", nargs="+", type=SnapshotPolicyValidator.minute_value, default=[0])
        
        __edit_parser.set_defaults( func=self.edit_snapshot_policy, format="tabular")
        
    def create_delete_parser(self, subparser):
        '''
        parser for the deletion of a snapshot policy
        '''
        
        __delete_parser = subparser.add_parser( "delete", help="Delete a specified snapshot policy from the system.  If the policy is still attached to any volumes, this call will fail.")
        self.add_format_arg( __delete_parser )
        
        __delete_parser.add_argument( "-" + AbstractPlugin.policy_id_str, help="The UUID of the snapshot policy you would like to delete.", required=True)
        
        __delete_parser.set_defaults( func=self.delete_snapshot_policy, format="tabular" )
        
    def create_attach_parser(self, subparser):
        '''
        create a parser for handling the attach command
        '''
        
        __attach_parser = subparser.add_parser( "attach", help="Attach the specified policy to a specified volume.  This will cause snapshot to happen on that volume in accordance with the policy.")
        self.add_format_arg( __attach_parser )
        
        __attach_parser.add_argument( "-" + AbstractPlugin.policy_id_str, help="The UUID of the policy that you wish to attach.", required=True)
        __attach_parser.add_argument( "-" + AbstractPlugin.volume_id_str, help="The UUID of the volume you would like to attach the policy to.", required=True)
        
        __attach_parser.set_defaults( func=self.attach_snapshot_policy, format="tabular")  
        
    def create_detach_parser(self, subparser):
        '''
        create a parser for handling the detach command
        '''
        
        __detach_parser = subparser.add_parser( "detach", help="Detach the specified policy to a specified volume.  This will cause snapshot to stop occurring on that volume in accordance with the policy.")
        self.add_format_arg( __detach_parser )
        
        __detach_parser.add_argument( "-" + AbstractPlugin.policy_id_str, help="The UUID of the policy that you wish to detach.", required=True)
        __detach_parser.add_argument( "-" + AbstractPlugin.volume_id_str, help="The UUID of the volume you would like to detach the policy from.", required=True)
        
        __detach_parser.set_defaults( func=self.detach_snapshot_policy, format="tabular")  
        
    # make the correct calls
    
    def list_snapshot_policies(self, args):
        '''
        List out all the snapshot policies in the system
        '''
        j_list = []
        
        #means we want only policies attached to this volume
        if ( AbstractPlugin.volume_id_str in  args and args[AbstractPlugin.volume_id_str] is not None):
            j_list = self.get_snapshot_policy_service().list_snapshot_policies_by_volume( args[AbstractPlugin.volume_id_str])
        else:
            j_list = self.get_snapshot_policy_service().list_snapshot_policies()
        
        if ( args[AbstractPlugin.format_str] == "json" ):
            j_policies = []
            
            for policy in j_list:
                j_policy = SnapshotPolicyConverter.to_json(policy)
                j_policy = json.loads( j_policy )
                j_policies.append( j_policy )
                
            ResponseWriter.writeJson( j_policies )
        else:
            cleaned = ResponseWriter.prep_snapshot_policy_for_table( self.session, j_list )
            ResponseWriter.writeTabularData( cleaned )
    
    def create_snapshot_policy(self, args):
        '''
        Takes the arguments and constructs a snapshot policy to send to the creation call
        '''
        policy = SnapshotPolicy()
        
        if ( args[AbstractPlugin.data_str] is None and args[AbstractPlugin.name_str] is None ):
            print "Either -data or -name must be present"
            return
        
        if ( args[AbstractPlugin.data_str] is not None ):
            policy = SnapshotPolicyConverter.build_snapshot_policy_from_json( args[AbstractPlugin.data_str])
        else:
            policy.name = args[AbstractPlugin.name_str]
            policy.retention = args[AbstractPlugin.retention_str]
            policy.recurrence_rule = RecurrenceRule()
            policy.recurrence_rule.frequency = args[AbstractPlugin.frequency_str]
            
            #if they specified a recurrence rule just use that
            if ( args[AbstractPlugin.recurrence_rule_str] is not None ):
                policy.recurrence_rule = RecurrenceRuleConverter.build_rule_from_json( json.loads( args[AbstractPlugin.recurrence_rule_str] ) )
            else:
                # day of week is not applicable if the frequency is daily
                if ( policy.recurrence_rule.frequency != "DAILY"):
                    policy.recurrence_rule.byday = args[AbstractPlugin.day_of_week_str]
                
                # only applicable to yearly jobs
                if ( policy.recurrence_rule.frequency == "YEARLY" ):
                    policy.recurrence_rule.bymonth = args[AbstractPlugin.month_str]
                    
                #only applicable to monthly or yearly jobs
                if ( policy.recurrence_rule.frequency == "YEARLY" or policy.recurrence_rule.frequency == "MONTHLY" ):    
                    policy.recurrence_rule.bymonthday = args[AbstractPlugin.day_of_month_str]
                    
                # only applicable to a yearly job
                if ( policy.recurrence_rule.frequency == "YEARLY" ):
                    policy.recurrence_rule.byyearday = args[AbstractPlugin.day_of_year_str]
                    
                policy.recurrence_rule.byhour = args[AbstractPlugin.hour_str]
                policy.recurrence_rule.byminute = args[AbstractPlugin.minute_str]
            
        self.get_snapshot_policy_service().create_snapshot_policy( policy )

        self.list_snapshot_policies(args)
    
    def edit_snapshot_policy(self, args):
        '''
        Take the arguments and construct a policy object to replace an existing one with
        '''
        policy = SnapshotPolicy()
        
        if ( args[AbstractPlugin.data_str] is not None ):
            policy = SnapshotPolicyConverter.build_snapshot_policy_from_json( args[AbstractPlugin.data_str])
        else:
            policy.id = args[AbstractPlugin.policy_id_str]
            policy.name = args[AbstractPlugin.name_str]
            policy.retention = args[AbstractPlugin.retention_str]
            policy.recurrence_rule = RecurrenceRule()
            policy.recurrence_rule.frequency = args[AbstractPlugin.frequency_str]
            
            #if they specified a recurrence rule just use that
            if ( args[AbstractPlugin.recurrence_rule_str] is not None ):
                policy.recurrence_rule = RecurrenceRuleConverter.build_rule_from_json( json.loads( args[AbstractPlugin.recurrence_rule_str] ) )
            else:
                # day of week is not applicable if the frequency is daily
                if ( policy.recurrence_rule.frequency != "DAILY"):
                    policy.recurrence_rule.byday = args[AbstractPlugin.day_of_week_str]
                
                # only applicable to yearly jobs
                if ( policy.recurrence_rule.frequency == "YEARLY" ):
                    policy.recurrence_rule.bymonth = args[AbstractPlugin.month_str]
                    
                #only applicable to monthly or yearly jobs
                if ( policy.recurrence_rule.frequency == "YEARLY" or policy.recurrence_rule.frequency == "MONTHLY" ):    
                    policy.recurrence_rule.bymonthday = args[AbstractPlugin.day_of_month_str]
                    
                # only applicable to a yearly job
                if ( policy.recurrence_rule.frequency == "YEARLY" ):
                    policy.recurrence_rule.byyearday = args[AbstractPlugin.day_of_year_str]
                    
                policy.recurrence_rule.byhour = args[AbstractPlugin.hour_str]
                policy.recurrence_rule.byminute = args[AbstractPlugin.minute_str]
            
        self.get_snapshot_policy_service().edit_snapshot_policy( policy )

        self.list_snapshot_policies(args)
    
    def delete_snapshot_policy(self, args):
        '''
        Take the arguments and make the call to delete the snapshot policy specified
        '''
        
        response = self.get_snapshot_policy_service().delete_snapshot_policy( args[AbstractPlugin.policy_id_str] )
            
        if ( response["status"].lower() == "ok" ):
            self.list_snapshot_policies(args)
            
            
    def attach_snapshot_policy(self, args):
        ''' 
        The the arguments and make the attach snapshot policy service call
        '''
        
        response = self.get_snapshot_policy_service().attach_snapshot_policy( args[AbstractPlugin.policy_id_str], args[AbstractPlugin.volume_id_str])
        
        if ( response["status"].lower() == "ok" ):
            self.list_snapshot_policies(args)
            
    def detach_snapshot_policy(self, args):
        '''
        Takes the arguments and makes the detach snapshot policy service call
        '''
        
        response = self.get_snapshot_policy_service().detach_snapshot_policy( args[AbstractPlugin.policy_id_str], args[AbstractPlugin.volume_id_str])
        
        if ( response["status"].lower() == "ok" ):
            self.list_snapshot_policies(args)
            
        