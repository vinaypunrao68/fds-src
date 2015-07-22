from services import volume_service
from services import response_writer
from model.volume.volume import Volume
from model.volume.snapshot import Snapshot
from utils.converters.volume.volume_converter import VolumeConverter
from abstract_plugin import AbstractPlugin
import json
import time
from utils.volume_validator import VolumeValidator
from utils.converters.volume.snapshot_converter import SnapshotConverter
from services.snapshot_policy_service import SnapshotPolicyService
from utils.converters.volume.snapshot_policy_converter import SnapshotPolicyConverter
from model.volume.settings.block_settings import BlockSettings
from model.common.size import Size
from model.volume.settings.object_settings import ObjectSettings
from model.fds_error import FdsError
from services.fds_auth import FdsAuth

class VolumePlugin( AbstractPlugin):
    '''
    Created on Apr 3, 2015
    
    This plugin will handle all call that deal with volumes.  Create, list, edit, attach snapshot policy etc.
    
    In order to run, it utilizes the volume service class and acts as a parsing pass through to acheive
    the various tasks
    
    @author: nate
    '''
    
    def __init__(self):
        AbstractPlugin.__init__(self)
    
    def build_parser(self, parentParser, session):
        '''
        @see: AbstractPlugin
        '''         
        
        self.session = session
        
        if not self.session.is_allowed( FdsAuth.VOL_MGMT ):
            return
        
        self.__volume_service = volume_service.VolumeService( self.session )
        
        self.__parser = parentParser.add_parser( "volume", help="All volume management operations" )
        self.__subparser = self.__parser.add_subparsers( help="The sub-commands that are available")
        
        self.create_list_command( self.__subparser )
        self.create_list_snapshot_policies_command( self.__subparser )
        self.create_create_command( self.__subparser )
        self.create_delete_command( self.__subparser )
        self.create_list_snapshots_command(self.__subparser)
        self.create_edit_command( self.__subparser )
        self.create_snapshot_command( self.__subparser )
        self.create_clone_command( self.__subparser )

    def detect_shortcut(self, args):
        '''
        @see: AbstractPlugin
        '''        

        # is it the listVolumes shortcut
        if ( args[0].endswith( "listVolumes" ) ):
            args.pop(0)
            args.insert( 0, "volume" )
            args.insert( 1, "list" )
            return args
        
        return None
        
    #parser creation        
    def create_list_command(self, subparser ):
        '''
        Configure the parser for the volume list command
        '''           
        __listParser = subparser.add_parser( "list", help="List all the volumes in the system" )
        __listParser.add_argument( "-" + AbstractPlugin.format_str, help="Specify the format that the result is printed as", choices=["json","tabular"], required=False )
        
#         indGroup = __listParser.add_argument_group( "Individual volume queries", "Indicate how to identify the volume that you are looking for." )
#         __listParserGroup = indGroup.add_mutually_exclusive_group()
        __listParser.add_argument( "-" + AbstractPlugin.volume_id_str, help="Specify a particular volume to list by UUID", default=None )
#         __listParserGroup.add_argument( "-" + AbstractPlugin.volume_name_str, help="Specify a particular volume to list by name" )
        __listParser.set_defaults( func=self.list_volumes, format="tabular" )
        

    def create_list_snapshot_policies_command(self, subparser):
        '''
        List all of the snapshot policies for a specified volume
        '''
        
        __listSp_parser = subparser.add_parser( "list_snapshot_policies", help="List the snapshot policies for a specific volume.")
        self.add_format_arg(__listSp_parser)
        __listSp_parser.add_argument( self.arg_str + AbstractPlugin.volume_id_str, help="The ID of the volume you'd like to list the policies for.", required=True)
        
        __listSp_parser.set_defaults( func=self.list_snapshot_policies, format="tabular")

    def create_create_command(self, subparser):
        '''
        Create the parser for the volume creation command
        '''
                 
        __createParser = subparser.add_parser( "create", help="Create a new volume" )
        __createParser.add_argument( self.arg_str + AbstractPlugin.format_str, help="Specify the format that the result is printed as", choices=["json","tabular"], required=False )
        __createParser.add_argument( self.arg_str + AbstractPlugin.data_str, help="JSON string representing the volume parameters desired for the new volume.  This argument will take precedence over all individual arguments.", default=None)
        __createParser.add_argument( self.arg_str + AbstractPlugin.name_str, help="The name of the volume", default=None )
        __createParser.add_argument( self.arg_str + AbstractPlugin.qos_preset_str, help="The ID of the quality of service preset you would like applied.  Take precedence over individually set items.", default=None)
        __createParser.add_argument( self.arg_str + AbstractPlugin.timeline_preset_str, help="The ID of the data protection preset you would like applied.  This will cause snapshot policies to be created and attached to this volume.", default=None)
        __createParser.add_argument( self.arg_str + AbstractPlugin.iops_limit_str, help="The IOPs limit for the volume.  0 = unlimited and is the default if not specified.", type=VolumeValidator.iops_limit, default=0, metavar="" )
        __createParser.add_argument( self.arg_str + AbstractPlugin.iops_guarantee_str, help="The IOPs guarantee for this volume.  0 = no guarantee and is the default if not specified.", type=VolumeValidator.iops_guarantee, default=0, metavar="" )
        __createParser.add_argument( self.arg_str + AbstractPlugin.priority_str, help="A value that indicates how to prioritize performance for this volume.  1 = highest priority, 10 = lowest.  Default value is 7.", type=VolumeValidator.priority, default=7, metavar="")
        __createParser.add_argument( self.arg_str + AbstractPlugin.type_str, help="The type of volume connector to use for this volume.", choices=["object", "block"], default="object")
        __createParser.add_argument( self.arg_str + AbstractPlugin.media_policy_str, help="The policy that will determine where the data will live over time.", choices=["HYBRID", "SSD", "HDD"], default="HDD")
        __createParser.add_argument( self.arg_str + AbstractPlugin.continuous_protection_str, help="A value (in seconds) for how long you want continuous rollback for this volume.  All values less than 24 hours will be set to 24 hours.", type=VolumeValidator.continuous_protection, default=86400, metavar="" )
        __createParser.add_argument( self.arg_str + AbstractPlugin.size_str, help="How large you would like the volume to be as a numerical value.  It will assume the value is in GB unless you specify the size_units.  NOTE: This is only applicable to Block volumes", type=VolumeValidator.size, default=10, metavar="" )
        __createParser.add_argument( self.arg_str + AbstractPlugin.size_unit_str, help="The units that should be applied to the size parameter.", choices=["MB","GB","TB"], default="GB")
        __createParser.add_argument( self.arg_str + AbstractPlugin.block_size_str, help="The block size you would like to use for block type volumes.  This value must be between 4KB and 8MB.  If it is not, the system default will be applied.", type=int, default=None)
        __createParser.add_argument( self.arg_str + AbstractPlugin.block_size_unit_str, help="The units that you wish the block size to be in.  The default is KB.", choices=["KB","MB"], default="KB")
        __createParser.add_argument( self.arg_str + AbstractPlugin.max_obj_size_str, help="The internal maximum size for one blob.  This is only applicable to OBJECT volumes and must be within 4KB and 8MB.  If it is not, the system default will be applied.", type=int, default=None)
        __createParser.add_argument( self.arg_str + AbstractPlugin.max_obj_size_unit_str, help="The units that you with the max object size to be in.  The default is MB.", choices=["B", "KB", "MB", "GB", "TB"], default="MB")
        
        __createParser.set_defaults( func=self.create_volume, format="tabular" )

    def create_edit_command(self, subparser):
        '''
        Create the parser for the edit command
        '''
        
        __editParser = subparser.add_parser( "edit", help="Edit the quality of service settings on your volume")
        __editParser.add_argument( self.arg_str + AbstractPlugin.format_str, help="Specify the format that the result is printed as", choices=["json","tabular"], required=False )
        __editGroup = __editParser.add_mutually_exclusive_group( required=True )
        __editGroup.add_argument( self.arg_str + AbstractPlugin.data_str, help="A JSON string representing the volume and parameters to change.  This argument will take precedence over all individual arguments.", default=None)
        __editGroup.add_argument( self.arg_str + AbstractPlugin.volume_id_str, help="The UUID of the volume you would like to edit.", default=None)
        __editParser.add_argument( self.arg_str + AbstractPlugin.qos_preset_str, help="The ID of the quality of service preset you would like applied.  Take precedence over individually set items.", default=None)
        __editParser.add_argument( self.arg_str + AbstractPlugin.timeline_preset_str, help="The ID of the timeline preset you would like applied.  This will cause snapshot policies to be created and attached to this volume.", default=None)
        __editParser.add_argument( self.arg_str + AbstractPlugin.iops_limit_str, help="The IOPs limit for the volume.  0 = unlimited.", type=VolumeValidator.iops_limit, default=None, metavar="" )
        __editParser.add_argument( self.arg_str + AbstractPlugin.iops_guarantee_str, help="The IOPs guarantee for this volume.  0 = no guarantee.", type=VolumeValidator.iops_guarantee, default=None, metavar="" )
        __editParser.add_argument( self.arg_str + AbstractPlugin.priority_str, help="A value that indicates how to prioritize performance for this volume.  1 = highest priority, 10 = lowest.", type=VolumeValidator.priority, default=None, metavar="")
#         __editParser.add_argument( "-media_policy", help="The policy that will determine where the data will live over time.", choices=["HYBRID_ONLY", "SSD_ONLY", "HDD_ONLY"], default=None)
        __editParser.add_argument( self.arg_str + AbstractPlugin.continuous_protection_str, help="A value (in seconds) for how long you want continuous rollback for this volume.  All values less than 24 hours will be set to 24 hours.", type=VolumeValidator.continuous_protection, default=None, metavar="" )
        
        __editParser.set_defaults( func=self.edit_volume, format="tabular")
        
    def create_clone_command(self, subparser):
        '''
        Create the parser for the clone command
        '''
        
        __cloneParser = subparser.add_parser( "clone", help="Create a clone of a volume from the current volume or a snapshot from the past.")
        __cloneParser.add_argument( "-" + AbstractPlugin.format_str, help="Specify the format that the result is printed as", choices=["json","tabular"], required=False )
        __cloneParser.add_argument( "-" + AbstractPlugin.name_str, help="The name of the resulting new volume.", required=True )
        __cloneParser.add_argument( "-" + AbstractPlugin.volume_id_str, help="The UUID of the volume from which the clone will be made.", required=True)
        
        __fromGroup = __cloneParser.add_mutually_exclusive_group( required=False )
        __fromGroup.add_argument( "-" + AbstractPlugin.time_str, help="The time (in seconds from the epoch) that you wish the clone to be made from.  The system will select the snapshot of the volume chosen which is nearest to this value.  If not specified, the default is to use the current time.", default=None, type=int)
        __fromGroup.add_argument( "-" + AbstractPlugin.snapshot_id_str, help="The UUID of the snapshot you would like to create the clone from.")
        
        __cloneParser.add_argument( self.arg_str + AbstractPlugin.qos_preset_str, help="The ID of the quality of service preset you would like to use.  This will take precedence over any other QoS settings.", default=None)
        __cloneParser.add_argument( self.arg_str + AbstractPlugin.timeline_preset_str, help="The ID of the data protection preset that you would like to be applied.  This will create and attach snapshot policies to the created volume.", default=None)
        __cloneParser.add_argument( "-" + AbstractPlugin.iops_limit_str, help="The IOPs maximum for the volume.  If not specified, the default will be the same as the parent volume.  0 = unlimited.", type=VolumeValidator.iops_limit, default=None, metavar="" )
        __cloneParser.add_argument( "-" + AbstractPlugin.iops_guarantee_str, help="The IOPs minimum for this volume.  If not specified, the default will be the same as the parent volume.  0 = no guarantee.", type=VolumeValidator.iops_guarantee, default=None, metavar="" )
        __cloneParser.add_argument( "-" + AbstractPlugin.priority_str, help="A value that indicates how to prioritize performance for this volume.  If not specified, the default will be the same as the parent volume.  1 = highest priority, 10 = lowest.", type=VolumeValidator.priority, default=None, metavar="")
        __cloneParser.add_argument( "-" + AbstractPlugin.continuous_protection_str, help="A value (in seconds) for how long you want continuous rollback for this volume.  If not specified, the default will be the same as the parent volume.  All values less than 24 hours will be set to 24 hours.", type=VolumeValidator.continuous_protection, default=None, metavar="" )
        __cloneParser.add_argument( "-" + AbstractPlugin.data_str, help="A JSON string containing the IOPs minimum, IOPs maximum, priority and continuous protection settings.", default=None )

        __cloneParser.set_defaults( func=self.clone_volume, format="tabular" )

    def create_delete_command(self, subparser):
        '''
        create a parser for the delete command
        '''
        
        __deleteParser = subparser.add_parser( "delete", help="Delete a specified volume." )
        __deleteParser.add_argument( "-" + AbstractPlugin.format_str, help="Specify the format that the result is printed as", choices=["json","tabular"], required=False )
        __deleteGroup = __deleteParser.add_mutually_exclusive_group( required=True)
        __deleteGroup.add_argument( "-" + AbstractPlugin.volume_id_str, help="The UUID of the volume to delete.")
        __deleteGroup.add_argument( "-" + AbstractPlugin.volume_name_str, help="The name of the volume to delete.  If the name is not unique, this call will fail.")
        
        __deleteParser.set_defaults( func=self.delete_volume, format="tabular")
        
    def create_snapshot_command(self, subparser):
        '''
        Create the parser for the create snapshot command
        '''
        __snapshotParser = subparser.add_parser( "create_snapshot", help="Create a snapshot from a specific volume.")
        __snapshotParser.add_argument( "-" + AbstractPlugin.format_str, help="Specify the format that the result is printed as", choices=["json","tabular"], required=False )
        __snapshotParser.add_argument( "-" + AbstractPlugin.name_str, help="The name to give this snapshot.", required=True)
        __snapshotParser.add_argument( "-" + AbstractPlugin.volume_id_str, help="The UUID of the volume that you'd like to take a snapshot of.", required=True)
        __snapshotParser.add_argument( "-" + AbstractPlugin.retention_str, help="The time (in seconds) that this snapshot will be retained.  0 = forever.", default=0, type=int )
        
        __snapshotParser.set_defaults( func=self.create_snapshot, format="tabular" )
        
    def create_list_snapshots_command(self, subparser):
        '''
        create the list snapshot command
        '''
        
        __listSnapsParser = subparser.add_parser( "list_snapshots", help="List all of the snapshots that exist for a specific volume.")
        __listSnapsParser.add_argument( "-" + AbstractPlugin.format_str, help="Specify the format that the result is printed as", choices=["json","tabular"], required=False )
        __listSnapsParser.add_argument( "-" + AbstractPlugin.volume_id_str, help="The UUID of the volume to list snapshots for.", required=True)
        
        __listSnapsParser.set_defaults( func=self.list_snapshots, format="tabular")
    
    #other class utilities
    def get_volume_service(self):
        '''
        Creates one instance of the volume service and it's accessed by this getter
        '''        
        return self.__volume_service           

        
# Area for actual calls and JSON manipulation        
         
    def list_volumes(self, args):
        '''
        Retrieve a list of volumes in accordance with the passed in arguments
        '''
        response = []
        
        if AbstractPlugin.volume_id_str in args and args[AbstractPlugin.volume_id_str] is not None:
            response = self.get_volume_service().get_volume(args[AbstractPlugin.volume_id_str])
            response = [response]
        else:
            response = self.get_volume_service().list_volumes()
        
        if isinstance( response, FdsError ):
            return
        
        if len( response ) == 0:
            print "\nNo volumes found."
        
        #write the volumes out
        if "format" in args  and args[AbstractPlugin.format_str] == "json":
            
            j_volumes = []
            
            for volume in response:
                j_volume = VolumeConverter.to_json(volume)
                j_volume = json.loads(j_volume)
                j_volumes.append( j_volume )
                
            response_writer.ResponseWriter.writeJson( j_volumes )
        else:
            #The tabular format is very poor for a volume object, so we need to remove some keys before display
            response = response_writer.ResponseWriter.prep_volume_for_table( self.session, response )
            response_writer.ResponseWriter.writeTabularData( response ) 
        

    def list_snapshot_policies(self, args):
        '''
        List out the policies attached to this volume
        '''
        snapshot_policy_service = SnapshotPolicyService(self.session)
        
        j_list = snapshot_policy_service.list_snapshot_policies( args[AbstractPlugin.volume_id_str])
        
        if isinstance( j_list, FdsError ):
            return
        
        if ( args[AbstractPlugin.format_str] == "json" ):
            j_policies = []
            
            for policy in j_list:
                j_policy = SnapshotPolicyConverter.to_json(policy)
                j_policy = json.loads( j_policy )
                j_policies.append( j_policy )
                
            response_writer.ResponseWriter.writeJson( j_policies )
        else:
            cleaned = response_writer.ResponseWriter.prep_snapshot_policy_for_table( self.session, j_list )
            response_writer.ResponseWriter.writeTabularData( cleaned )        

    def create_volume(self, args):
        '''
        Create a new volume.  The arguments are not all necessary (@see: Volume __init__) but the user
        must specify a name either in the -data construct or the -name argument
        '''        
        volume = Volume()
        
        if ( args[AbstractPlugin.data_str] is None and args[AbstractPlugin.name_str] is None ):
            print "Either -data or -name must be present"
            return
        
        #if -data exists all other arguments will be ignored!
        if ( args[AbstractPlugin.data_str] is not None ):
            jsonData = json.loads( args[AbstractPlugin.data_str] )
            volume = VolumeConverter.build_volume_from_json( jsonData )
        # build the volume object from the arguments
        else:
       
            volume.name = args[AbstractPlugin.name_str]
            
            volume.type = args[AbstractPlugin.type_str]
            volume.media_policy = args[AbstractPlugin.media_policy_str]
            
            if ( volume.type.lower() == "block" ):
                volume.settings = BlockSettings()
                volume.settings.capacity = Size( size=args[AbstractPlugin.size_str], unit=args[AbstractPlugin.size_unit_str] )
                
                if args[AbstractPlugin.max_obj_size_str] is not None:
                    print "The argument " + AbstractPlugin.max_obj_size_str + " is not applicable to block volumes.  Use " + AbstractPlugin.block_size_str + " instead."
                    return
                
                if args[AbstractPlugin.block_size_str] is not None:
                    block_size = Size( size=args[AbstractPlugin.block_size_str], unit=args[AbstractPlugin.block_size_unit_str])
                    
                    if block_size.get_bytes() < (4*1024) or block_size.get_bytes() > (8*pow(1024,2)):
                        print "Warning: The block size you entered is outside the bounds of 4KB and 8MB.  The actual value will be the system default (typically 128KB)"
                
                    volume.settings.block_size = block_size
            else:
                volume.settings = ObjectSettings()
                
                if args[AbstractPlugin.block_size_str] is not None:
                    print "The argument " + AbstractPlugin.block_size_str + " is not applicable to block volumes.  Use " + AbstractPlugin.max_obj_size_str + " instead."
                    return

                if args[AbstractPlugin.max_obj_size_str] is not None:
                    obj_size = Size( size=args[AbstractPlugin.max_obj_size_str], unit=args[AbstractPlugin.max_obj_size_unit_str])
                
                    if obj_size.get_bytes() < (4*1024) or obj_size.get_bytes() > (8*pow(1024,2)):
                        print "Warning: The maximum object size you entered is outside the bounds of 4KB and 8MB.  The actual value will be the system default (typically 2MB)"
                
                    volume.settings.max_object_size = obj_size
            
            # deal with the QOS preset selection if there was one
            if args[AbstractPlugin.qos_preset_str] != None:
                qos_preset = self.get_volume_service().get_qos_presets(preset_id=args[AbstractPlugin.qos_preset_str])
                
                if len(qos_preset) >= 1:
                    qos_preset = qos_preset[0]
                    
                    volume.qos_policy.preset_id = qos_preset.id
                    volume.qos_policy.iops_max = qos_preset.iops_limit
                    volume.qos_policy.iops_min = qos_preset.iops_guarantee
                    volume.qos_policy.priority = qos_preset.priority
            else:                
                volume.qos_policy.iops_min = args[AbstractPlugin.iops_guarantee_str]
                volume.qos_policy.iops_max = args[AbstractPlugin.iops_limit_str]
                volume.qos_policy.priority = args[AbstractPlugin.priority_str]
                
            # deal with the continuous protection arg in the timeline preset if specified
            t_preset = None
            if args[AbstractPlugin.timeline_preset_str] != None:
                t_preset = self.get_volume_service().get_data_protection_presets(preset_id=args[AbstractPlugin.timeline_preset_str])[0]
                
                volume.data_protection_policy = t_preset
                volume.data_protection_policy.preset_id = t_preset.id
            else:
                volume.data_protection_policy.commit_log_retention = args[AbstractPlugin.continuous_protection_str]
        
        response = self.get_volume_service().create_volume( volume )
        
        if isinstance(response, Volume):
            self.list_volumes(args)
            
        return
    
    def edit_volume(self, args):
        '''
        Edit an existing volume.  The arguments are not all necessary but the user must specify
        something to uniquely identify the volume
        '''
        
        volume = None
        isFromData = False
        
        if ( args[AbstractPlugin.data_str] is not None):
            isFromData = True
            jsonData = json.loads( args[AbstractPlugin.data_str] )
            volume =  VolumeConverter.build_volume_from_json( jsonData )  
        elif ( args[AbstractPlugin.volume_id_str] is not None ):
            volume = self.get_volume_service().get_volume( args[AbstractPlugin.volume_id_str] )
               
        if ( volume.id is None ):
            print "Could not find a volume that matched your entry.\n"
            return       
           
        if ( args[AbstractPlugin.iops_guarantee_str] is not None and isFromData is False ):
            volume.qos_policy.iops_min = args[AbstractPlugin.iops_guarantee_str]
            
        if ( args[AbstractPlugin.iops_limit_str] is not None and isFromData is False):
            volume.qos_policy.iops_max = args[AbstractPlugin.iops_limit_str]
            
        if ( args[AbstractPlugin.priority_str] is not None and isFromData is False):
            volume.qos_policy.priority = args[AbstractPlugin.priority_str]
            
        if ( args[AbstractPlugin.continuous_protection_str] is not None and isFromData is False):
            volume.data_protection_policy.commit_log_retention = args[AbstractPlugin.continuous_protection_str]      
            
        #use the qos preset if it was provided
        if args[AbstractPlugin.qos_preset_str] != None:
            qos_preset = self.get_volume_service().get_qos_presets( args[AbstractPlugin.qos_preset_str] )
            
            if len(qos_preset) >= 1:
                qos_preset = qos_preset[0]
                volume.qos_policy.preset_id = qos_preset.id
                volume.qos_policy.iops_min = qos_preset.iops_guarantee
                volume.qos_policy.iops_max = qos_preset.iops_limit
                volume.qos_policy.priority = qos_preset.priority
            
        #if a timeline preset is provided we need to do the following
        # 1.  get a list of all attached policies
        # 2.  If the policy has the volume id_TIMELINE in the name we need to delete the policy
        # 3.  Create the policies like we do in the create method and attach them
        if args[AbstractPlugin.timeline_preset_str] != None:
            t_preset = self.get_volume_service().get_data_protection_presets(args[AbstractPlugin.timeline_preset_str])
            
            if len( t_preset ) >= 1:
                t_preset = t_preset[0]
                
                volume.data_protection_policy.commit_log_retention = t_preset.commit_log_retention
                
                #get attached policies
                policies = volume.data_protection_policy.snapshot_policies
                n_policies = []
                
                #clean up the current policy assignments
                for policy in policies:
                    
                    if not policy.type.startswith( volume.id + "SYSTEM_TIMELINE" ):
                        n_policies.append( policy )
                        
                volume.data_protection_policy.snapshot_policies = t_preset.snapshot_policies
                
                for policy in n_policies:
                    volume.data_protection_policy.snapshot_policies.append( policy ) 
                        
            
        response = self.get_volume_service().edit_volume( volume );
        
        if isinstance(response, Volume):
            args = [ args[AbstractPlugin.format_str]]
            self.list_volumes( args )                 
            
    def clone_volume(self, args):
        '''
        Clone a volume from the argument list
        '''
        #now
        fromTime = int(time.time())
        
        if ( args[AbstractPlugin.time_str] is not None ):
            fromTime = args[AbstractPlugin.time_str]
        
        volume = self.get_volume_service().get_volume( args[AbstractPlugin.volume_id_str] )
        
        if ( volume is None ):
            print "Could not find a volume associated with the input parameters."
            return
        
        iops_guarantee = volume.qos_policy.iops_min
        iops_limit = volume.qos_policy.iops_max
        priority = volume.qos_policy.priority
        continuous_protection = volume.data_protection_policy.commit_log_retention
        
        if ( args[AbstractPlugin.iops_guarantee_str] is not None ):
            iops_guarantee = args[AbstractPlugin.iops_guarantee_str]
        
        if ( args[AbstractPlugin.iops_limit_str] is not None):
            iops_limit = args[AbstractPlugin.iops_limit_str]
            
        if ( args[AbstractPlugin.priority_str] is not None):
            priority = args[AbstractPlugin.priority_str]
            
        if ( args[AbstractPlugin.continuous_protection_str] is not None):
            continuous_protection = args[AbstractPlugin.continuous_protection_str]
        
        if ( args[AbstractPlugin.data_str] is not None ):
            jsonData = json.loads( args[AbstractPlugin.data_str] )
            
            qosData = jsonData["qosPolicy"]
            dataProtection = jsonData["dataProtectionPolicy"]
            
            if ( qosData["priority"] is not None ):
                priority = qosData["priority"]
                
            if ( qosData["iopsMin"] is not None ):
                iops_guarantee = qosData["iopsMin"]
                
            if ( qosData["iopsMax"] is not None ):
                iops_limit = qosData["iopsMax"]
                
            if ( dataProtection["commitLogRetention"] is not None ):
                continuous_protection = dataProtection["commitLogRetention"]
                
        if args[AbstractPlugin.qos_preset_str] != None:
            qos_preset = self.get_volume_service().get_qos_presets(args[AbstractPlugin.qos_preset_str])
            
            if len( qos_preset ) >= 1:
                qos_preset = qos_preset[0]
                iops_guarantee = qos_preset.iops_min
                iops_limit = qos_preset.iops_max
                priority = qos_preset.priority     
                volume.qos_policy.preset_id = qos_preset.id   
        
        #have to get continuous from a potential timeline preset
        t_preset = None
        if args[AbstractPlugin.timeline_preset_str] != None:
            t_preset = self.get_volume_service().get_data_protection_presets(args[AbstractPlugin.timeline_preset_str])
            
            if len( t_preset ) >= 1:
                t_preset = t_preset[0]
                
                continuous_protection = t_preset.commit_log_retention
        
        volume.qos_policy.iops_min = iops_guarantee
        volume.qos_policy.iops_max = iops_limit
        volume.qos_policy.priority = priority
        volume.data_protection_policy.commit_log_retention = continuous_protection 
        
        volume.name = args[AbstractPlugin.name_str]       
        
        # one URL when snapshot was chosen, a different one if the time / volume was chosen
        new_volume = None
        if ( args[AbstractPlugin.snapshot_id_str] is not None ):
            new_volume = self.get_volume_service().clone_from_snapshot_id( volume, args[AbstractPlugin.snapshot_id_str] )
        else:
            new_volume = self.get_volume_service().clone_from_timeline( volume, fromTime )
            
        if isinstance(new_volume, Volume):
            
            #if there was a timeline preset included, create and attach those policies now
            if t_preset is not None:
                
                for policy in t_preset.policies:
                    volume.data_protection_policy.snapshot_policies.append( policy ) 
                    
                volume.data_protection_policy.preset_id = t_preset.id               
            
            print "Volume cloned successfully."
            args = [args[AbstractPlugin.format_str]]
            self.list_volumes( args );
    
    def delete_volume(self, args):
        '''
        Delete the indicated volume
        '''
        vol_id = args[AbstractPlugin.volume_id_str]
        
        if AbstractPlugin.volume_name_str in args and args[AbstractPlugin.volume_name_str] is not None:
            volume = self.get_volume_service().find_volume_by_name( args[AbstractPlugin.volume_name_str])
            vol_id = volume.id
        
        response = self.get_volume_service().delete_volume( vol_id )
        
        if not isinstance( response, FdsError ):
            print 'Deletion request completed successfully.'
            args = [args[AbstractPlugin.format_str]]
            self.list_volumes(args)
            
    def create_snapshot(self, args):
        '''
        Create a snapshot for a volume
        '''
        
        volume = self.get_volume_service().get_volume( args[AbstractPlugin.volume_id_str] )
            
        if ( volume is None ):
            print "No volume found with the specified identification.\n"
            return
        
        snapshot = Snapshot()
        
        snapshot.name = args[AbstractPlugin.name_str]
        snapshot.retention = args[AbstractPlugin.retention_str]
        snapshot.volume_id = volume.id
        
        response = self.get_volume_service().create_snapshot( snapshot )
        
        if isinstance(response, Snapshot):
            self.list_snapshots(args)
         
    def list_snapshots(self, args):
        '''
        List snapshots for this volume
        '''
        volId = args[AbstractPlugin.volume_id_str]
            
        response = self.get_volume_service().list_snapshots(volId)
        
        if isinstance( response, FdsError ):
            return
        
        if ( len( response ) == 0 ):
            print "No snapshots found for volume with ID " + volId;
        
        #print it all out
        if "format" in args  and args[AbstractPlugin.format_str] == "json":
            
            j_snapshots = []
            
            for snapshot in response:
                j_snapshot = SnapshotConverter.to_json(snapshot)
                j_snapshot = json.loads( j_snapshot )
                j_snapshots.append( j_snapshot )
                
            response_writer.ResponseWriter.writeJson( j_snapshots )
        else:
            resultList = response_writer.ResponseWriter.prep_snapshot_for_table( self.session, response)
            response_writer.ResponseWriter.writeTabularData( resultList )  
            
        
    
