import json

from model.volume.snapshot_policy import SnapshotPolicy
from model.volume.recurrence_rule import RecurrenceRule
from utils.converters.volume.snapshot_policy_converter import SnapshotPolicyConverter
from utils.snapshot_policy_validator import SnapshotPolicyValidator
from utils.converters.volume.recurrence_rule_converter import RecurrenceRuleConverter
from services.snapshot_policy_service import SnapshotPolicyService
from services.response_writer import ResponseWriter
from plugins.abstract_plugin import AbstractPlugin
from model.fds_error import FdsError
from services.fds_auth import FdsAuth


class SnapshotPolicyPlugin( AbstractPlugin):
    '''
    Created on Apr 13, 2015
    
    A plugin to handle all the parsing for snapshot policy management
    and to route those options to the appropriate snapshot policy 
    management calls
    
    @author: nate
    '''    
    
    
    def __init__(self):
        AbstractPlugin.__init__(self)  
    
    '''
    @see: AbstractPlugin
    '''
    def build_parser(self, parentParser, session): 
        
        self.session = session
       
        if not self.session.is_allowed( FdsAuth.VOL_MGMT ):
            return
        
        self.__sp_service = SnapshotPolicyService( self.session )  
 
        
        self.__parser = parentParser.add_parser( "snapshot_policy", help="Create, edit, delete and manipulate snapshot policies" )
        self.__subparser = self.__parser.add_subparsers( help="The sub-commands that are available")
        
        self.create_create_parser(self.__subparser)
        self.create_edit_parser(self.__subparser)
        self.create_list_parser(self.__subparser)
        self.create_delete_parser(self.__subparser)
     
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
        
        __list_parser.add_argument( "-" + AbstractPlugin.volume_id_str, help="The UUID of the volume for which you would like to list the snapshot policies", default=None, required=True)
        
        __list_parser.set_defaults( func=self.list_snapshot_policies, format="tabular")
    
    def create_create_parser(self, subparser):
        '''
        Create a parser for the creation of a snapshot policy
        '''
        
        __create_parser = subparser.add_parser( "create", help="Create a new snapshot policy.")
        self.add_format_arg( __create_parser )
        
        __create_parser.add_argument( "-" + AbstractPlugin.data_str, help="A JSON formatted string that defines your policy entirely.  If this is present all other arguments will be ignored.", default=None )
        __create_parser.add_argument( "-" + AbstractPlugin.name_str, help="The name of the policy you are creating.", default=None)
        __create_parser.add_argument( "-" + AbstractPlugin.volume_id_str, help="The UUID of the volume that this policy will be attached to.", required=True)
        __create_parser.add_argument( "-" + AbstractPlugin.retention_str, help="The time (in seconds) that you want to keep snapshots that are created with this policy. 0 = forever and will be the default is not specified.", type=int, default=0)
        __create_parser.add_argument( "-" + AbstractPlugin.recurrence_rule_str, help="The iCal format recurrence rule you would like this policy to follow. http://www.kanzaki.com/docs/ical/rrule.html")
        __create_parser.add_argument( "-" + AbstractPlugin.frequency_str, help="The frequency for which you would like snapshots to be taken on volumes this policy is applied to.  DAILY is the default.", choices=["DAILY", "HOURLY", "WEEKLY", "MONTHLY", "YEARLY"], default="DAILY")
        __create_parser.add_argument( "-" + AbstractPlugin.day_of_week_str, help="The day(s) of the week you would like the policy to run (when applicable).  The default value is Sunday.", choices=["SU","MO","TU","WE","TH","FR","SA"], nargs="+", default=["SU"])
        __create_parser.add_argument( "-" + AbstractPlugin.day_of_month_str, help="The number day you would like the policy to run.", nargs="+", type=SnapshotPolicyValidator.day_of_month_values)
        __create_parser.add_argument( "-" + AbstractPlugin.day_of_year_str, help="The number day you would like the policy to run.", nargs="+", type=SnapshotPolicyValidator.day_of_year_values)
        __create_parser.add_argument( "-" + AbstractPlugin.month_str, help="The month you would like this policy to run.", nargs="+", type=SnapshotPolicyValidator.month_value)
        __create_parser.add_argument( "-" + AbstractPlugin.hour_str, help="The hour(s) of the day you wish this policy to run.  Midnight is the default.", nargs="+", type=SnapshotPolicyValidator.hour_value, default=[0])
        __create_parser.add_argument( "-" + AbstractPlugin.minute_str, help="The minute(s) of the day you wish this policy to run.  On the hour is the default.", nargs="+", type=SnapshotPolicyValidator.minute_value, default=[0])
        
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
        
        __edit_parser.add_argument( "-" + AbstractPlugin.volume_id_str, help="The UUID of the volume that this policy resides on.", required=True)
        
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
        __delete_parser.add_argument( "-" + AbstractPlugin.volume_id_str, help="The UUID of the volume that this policy is attached to.", required=True)
        
        __delete_parser.set_defaults( func=self.delete_snapshot_policy, format="tabular" ) 
        
    # make the correct calls
    
    def list_snapshot_policies(self, args):
        '''
        List out all the snapshot policies in the system
        '''
        
        #means we want only policies attached to this volume
        j_list = self.get_snapshot_policy_service().list_snapshot_policies( args[AbstractPlugin.volume_id_str])
        
        if isinstance(j_list, FdsError):
            return None
        
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
            print("Either -data or -name must be present")
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
            
        new_policy = self.get_snapshot_policy_service().create_snapshot_policy( args[AbstractPlugin.volume_id_str], policy )

        if isinstance( new_policy, SnapshotPolicy):
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
            
        new_policy = self.get_snapshot_policy_service().edit_snapshot_policy( policy )

        if isinstance(new_policy, SnapshotPolicy):
            self.list_snapshot_policies(args)
    
    def delete_snapshot_policy(self, args):
        '''
        Take the arguments and make the call to delete the snapshot policy specified
        '''
        
        response = self.get_snapshot_policy_service().delete_snapshot_policy( args[AbstractPlugin.volume_id_str], args[AbstractPlugin.policy_id_str] )
            
        if not isinstance( response, FdsError ):
            self.list_snapshot_policies(args)




