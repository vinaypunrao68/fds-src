from fds.services import volume_service
from fds.services import response_writer
from fds.model.volume import Volume
from fds.model.snapshot import Snapshot
from fds.utils.volume_converter import VolumeConverter
from collections import OrderedDict

import abstract_plugin
import json
import time
from fds.utils.volume_validator import VolumeValidator
from fds.utils.snapshot_converter import SnapshotConverter

class VolumePlugin( abstract_plugin.AbstractPlugin):
    '''
    Created on Apr 3, 2015
    
    This plugin will handle all call that deal with volumes.  Create, list, edit, attach snapshot policy etc.
    
    In order to run, it utilizes the volume service class and acts as a parsing pass through to acheive
    the various tasks
    
    @author: nate
    '''
    
    def __init__(self, session):
        abstract_plugin.AbstractPlugin.__init__(self, session)
    
    def build_parser(self, parentParser, session):
        '''
        @see: AbstractPlugin
        '''         
        
        if ( self.session.is_allowed( "VOL_MGMT" ) == False ):
            return
        
        self.__volume_service = volume_service.VolumeService( session )
        
        self.__parser = parentParser.add_parser( "volume", help="All volume management operations" )
        self.__subparser = self.__parser.add_subparsers( help="The sub-commands that are available")
        
        self.create_list_command( self.__subparser )
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
        __listParser.add_argument( "-format", help="Specify the format that the result is printed as", choices=["json","tabular"], required=False )
        
        indGroup = __listParser.add_argument_group( "Individual volume queries", "Indicate how to identify the volume that you are looking for." )
        __listParserGroup = indGroup.add_mutually_exclusive_group()
        __listParserGroup.add_argument( "-volume_id", help="Specify a particular volume to list by UUID" )
        __listParserGroup.add_argument( "-volume_name", help="Specify a particular volume to list by name" )
        __listParser.set_defaults( func=self.list_volumes, format="tabular" )
        

    def create_create_command(self, subparser):
        '''
        Create the parser for the volume creation command
        '''
                 
        __createParser = subparser.add_parser( "create", help="Create a new volume" )
        __createParser.add_argument( "-format", help="Specify the format that the result is printed as", choices=["json","tabular"], required=False )
        __createParser.add_argument( "-data", help="JSON string representing the volume parameters desired for the new volume.  This argument will take precedence over all individual arguments.", default=None)
        __createParser.add_argument( "-name", help="The name of the volume", default=None )
        __createParser.add_argument( "-iops_limit", help="The IOPs limit for the volume.  0 = unlimited and is the default if not specified.", type=VolumeValidator.iops_limit, default=0, metavar="" )
        __createParser.add_argument( "-iops_guarantee", help="The IOPs guarantee for this volume.  0 = no guarantee and is the default if not specified.", type=VolumeValidator.iops_guarantee, default=0, metavar="" )
        __createParser.add_argument( "-priority", help="A value that indicates how to prioritize performance for this volume.  1 = highest priority, 10 = lowest.  Default value is 7.", type=VolumeValidator.priority, default=7, metavar="")
        __createParser.add_argument( "-type", help="The type of volume connector to use for this volume.", choices=["object", "block"], default="object")
        __createParser.add_argument( "-media_policy", help="The policy that will determine where the data will live over time.", choices=["HYBRID_ONLY", "SSD_ONLY", "HDD_ONLY"], default="HDD_ONLY")
        __createParser.add_argument( "-continuous_protection", help="A value (in seconds) for how long you want continuous rollback for this volume.  All values less than 24 hours will be set to 24 hours.", type=VolumeValidator.continuous_protection, default=86400, metavar="" )
        __createParser.add_argument( "-size", help="How large you would like the volume to be as a numerical value.  It will assume the value is in GB unless you specify the size_units.  NOTE: This is only applicable to Block volumes", type=VolumeValidator.size, default=10, metavar="" )
        __createParser.add_argument( "-size_unit", help="The units that should be applied to the size parameter.", choices=["MB","GB","TB"], default="GB")
        
        __createParser.set_defaults( func=self.create_volume, format="tabular" )

    def create_edit_command(self, subparser):
        '''
        Create the parser for the edit command
        '''
        
        __editParser = subparser.add_parser( "edit", help="Edit the quality of service settings on your volume")
        __editParser.add_argument( "-format", help="Specify the format that the result is printed as", choices=["json","tabular"], required=False )
        __editGroup = __editParser.add_mutually_exclusive_group( required=True )
        __editGroup.add_argument( "-data", help="A JSON string representing the volume quality of service parameters.  This argument will take precedence over all individual arguments.", default=None)
        __editGroup.add_argument( "-volume_name", help="The name of the volume you would like to edit.", default=None)
        __editGroup.add_argument( "-volume_id", help="The UUID of the volume you would like to edit.", default=None)
        __editParser.add_argument( "-iops_limit", help="The IOPs limit for the volume.  0 = unlimited and is the default if not specified.", type=VolumeValidator.iops_limit, default=None, metavar="" )
        __editParser.add_argument( "-iops_guarantee", help="The IOPs guarantee for this volume.  0 = no guarantee and is the default if not specified.", type=VolumeValidator.iops_guarantee, default=None, metavar="" )
        __editParser.add_argument( "-priority", help="A value that indicates how to prioritize performance for this volume.  1 = highest priority, 10 = lowest.  Default value is 7.", type=VolumeValidator.priority, default=None, metavar="")
#         __editParser.add_argument( "-media_policy", help="The policy that will determine where the data will live over time.", choices=["HYBRID_ONLY", "SSD_ONLY", "HDD_ONLY"], default=None)
        __editParser.add_argument( "-continuous_protection", help="A value (in seconds) for how long you want continuous rollback for this volume.  All values less than 24 hours will be set to 24 hours.", type=VolumeValidator.continuous_protection, default=None, metavar="" )
        
        __editParser.set_defaults( func=self.edit_volume, format="tabular")
        
    def create_clone_command(self, subparser):
        '''
        Create the parser for the clone command
        '''
        
        __cloneParser = subparser.add_parser( "clone", help="Create a clone of a volume from the current volume or a snapshot from the past.")
        __cloneParser.add_argument( "-format", help="Specify the format that the result is printed as", choices=["json","tabular"], required=False )
        __cloneParser.add_argument( "-name", help="The name of the resulting new volume.", required=True )
        __idGroup = __cloneParser.add_mutually_exclusive_group( required=True )
        __idGroup.add_argument( "-volume_name", help="The name of the volume from which the clone will be made.")
        __idGroup.add_argument( "-volume_id", help="The UUID of the volume from which the clone will be made.")
        __idGroup.add_argument( "-snapshot_id", help="The UUID of the snapshot you would like to create the clone from.")
        __cloneParser.add_argument( "-time", help="The time (in seconds from the epoch) that you wish the clone to be made from.  The system will select the snapshot of the volume chosen which is nearest to this value.  If not specified, the default is to use the current time.", default=0, type=int)
        __cloneParser.add_argument( "-iops_limit", help="The IOPs limit for the volume.  If not specified, the default will be the same as the parent volume.  0 = unlimited.", type=VolumeValidator.iops_limit, default=None, metavar="" )
        __cloneParser.add_argument( "-iops_guarantee", help="The IOPs guarantee for this volume.  If not specified, the default will be the same as the parent volume.  0 = no guarantee.", type=VolumeValidator.iops_guarantee, default=None, metavar="" )
        __cloneParser.add_argument( "-priority", help="A value that indicates how to prioritize performance for this volume.  If not specified, the default will be the same as the parent volume.  1 = highest priority, 10 = lowest.", type=VolumeValidator.priority, default=None, metavar="")
        __cloneParser.add_argument( "-continuous_protection", help="A value (in seconds) for how long you want continuous rollback for this volume.  If not specified, the default will be the same as the parent volume.  All values less than 24 hours will be set to 24 hours.", type=VolumeValidator.continuous_protection, default=None, metavar="" )
        __cloneParser.add_argument( "-data", help="A JSON string containing the IOPs guarantee, IOPs limit, priority and continuous protection settings.", default=None )

        __cloneParser.set_defaults( func=self.clone_volume, format="tabular" )

    def create_delete_command(self, subparser):
        '''
        create a parser for the delete command
        '''
        
        __deleteParser = subparser.add_parser( "delete", help="Delete a specified volume." )
        __deleteParser.add_argument( "-format", help="Specify the format that the result is printed as", choices=["json","tabular"], required=False )
        __deleteGroup = __deleteParser.add_mutually_exclusive_group( required=True)
        __deleteGroup.add_argument( "-volume_id", help="The UUID of the volume to delete.")
        __deleteGroup.add_argument( "-volume_name", help="The name of the volume to delete.  If the name is not unique, this call will fail.")
        
        __deleteParser.set_defaults( func=self.delete_volume, format="tabular")
        
    def create_snapshot_command(self, subparser):
        '''
        Create the parser for the create snapshot command
        '''
        __snapshotParser = subparser.add_parser( "create_snapshot", help="Create a snapshot from a specific volume.")
        __snapshotParser.add_argument( "-format", help="Specify the format that the result is printed as", choices=["json","tabular"], required=False )
        __snapshotParser.add_argument( "-name", help="The name to give this snapshot.", required=True)
        __snapshotGroup = __snapshotParser.add_mutually_exclusive_group( required=True )
        __snapshotGroup.add_argument( "-volume_name", help="The name of the volume that you'd like to take a snapshot of.")
        __snapshotGroup.add_argument( "-volume_id", help="The UUID of the volume that you'd like to take a snapshot of.")
        __snapshotParser.add_argument( "-retention", help="The time (in seconds) that this snapshot will be retained.  0 = forever.", default=0, type=int )
        
        __snapshotGroup.set_defaults( func=self.create_snapshot, format="tabular" )
        
    def create_list_snapshots_command(self, subparser):
        '''
        create the list snapshot command
        '''
        
        __listSnapsParser = subparser.add_parser( "list_snapshots", help="List all of the snapshots that exist for a specific volume.")
        __listSnapsParser.add_argument( "-format", help="Specify the format that the result is printed as", choices=["json","tabular"], required=False )
        __listGroup = __listSnapsParser.add_mutually_exclusive_group( required=True )
        __listGroup.add_argument( "-volume_id", help="The UUID of the volume to list snapshots for.")
        __listGroup.add_argument( "-volume_name", help="The name of the volume to list snapshots for.")
        
        __listGroup.set_defaults( func=self.list_snapshots, format="tabular")
    
    #other class utilities
    def get_volume_service(self):
        '''
        Creates one instance of the volume service and it's accessed by this getter
        '''        
        return self.__volume_service           

    def clean_response(self, response):
        '''
        Flatten the volume JSON object dictionary so it's more user friendly - mainly
        for use with the tabular print option
        '''        
        
        cleanedResponses = []
        
        # Some calls return a singular volume instead of a list, but if we put it in a list we can re-use this algorithm
        if ( isinstance( response, dict ) ):
            response = [response]
        
        for jVolume in response:
            volume = VolumeConverter.build_volume_from_json( jVolume )
            
            #figure out what to show for last firebreak occurrence
            lastFirebreak = volume.last_capacity_firebreak
            lastFirebreakType = "Capacity"
            
            if ( volume.last_performance_firebreak > lastFirebreak ):
                lastFirebreak = volume.last_performance_firebreak
                lastFirebreakType = "Performance"
                
            if ( lastFirebreak == 0 ):
                lastFirebreak = "Never"
                lastFirebreakType = ""
            else:
                lastFirebreak = time.localtime( lastFirebreak )
                lastFirebreak = time.strftime( "%c", lastFirebreak )
            
            #sanitize the IOPs guarantee value
            iopsMin = volume.iops_guarantee
            
            if ( iopsMin == 0 ):
                iopsMin = "Unlimited"
                
            #sanitize the IOPs limit
            iopsLimit = volume.iops_limit
            
            if ( iopsLimit == 0 ):
                iopsLimit = "None"
            
            fields = [];
            
            fields.append(("ID", volume.id))
            fields.append(("Name", volume.name))
            
            if ( self.session.is_allowed( "TENANT_MGMT" ) ):
                fields.append(("Tenant ID", volume.tenant_id))
            
            fields.append(("State", volume.state))
            fields.append(("Type", volume.type))
            fields.append(("Usage", str(volume.current_size) + " " + volume.current_units))
            fields.append(("Last Firebreak Type", lastFirebreakType))
            fields.append(("Last Firebreak", lastFirebreak))
            fields.append(("Priority", volume.priority))
            fields.append(("IOPs Guarantee", iopsMin))
            fields.append(("IOPs Limit", iopsLimit))
            fields.append(("Media Policy", volume.media_policy))
            
            ov = OrderedDict( fields )
            cleanedResponses.append( ov )
        #end of for loop 
            
        return cleanedResponses
        
# Area for actual calls and JSON manipulation        
         
    def list_volumes(self, args):
        '''
        Retrieve a list of volumes in accordance with the passed in arguments
        '''
        response = self.get_volume_service().list_volumes()
        
        if ( len( response ) == 0 ):
            print "\nNo volumes found."
        
        #write the volumes out
        if "format" in args  and args["format"] == "json":
            response_writer.ResponseWriter.writeJson( response )
        else:
            #The tabular format is very poor for a volume object, so we need to remove some keys before display
            response = self.clean_response( response )
            response_writer.ResponseWriter.writeTabularData( response ) 
        

    def create_volume(self, args):
        '''
        Create a new volume.  The arguments are not all necessary (@see: Volume __init__) but the user
        must specify a name either in the -data construct or the -name argument
        '''        
        volume = None
        
        if ( args["data"] == None and args["name"] == None ):
            print "Either -data or -name must be present"
            return
        
        #if -data exists all other arguments will be ignored!
        if ( args["data"] != None ):
            jsonData = json.loads( args["data"] )
            volume = VolumeConverter.build_volume_from_json( jsonData )
        # build the volume object from the arguments
        else:
            volume = Volume()
            volume.name = args["name"]
            volume.continuous_protection = args["continuous_protection"]
            volume.iops_guarantee = args["iops_guarantee"]
            volume.iops_limit = args["iops_limit"]
            volume.priority = args["priority"]
            volume.type = args["type"]
            volume.media_policy = args["media_policy"]
            
            if ( volume.type == "block" ):
                volume.current_size = args["size"]
                volume.current_units = args["size_unit"]
        
        volume = self.get_volume_service().create_volume( volume )
        
        if ( volume != None ):
            self.list_volumes(args)
            
        return
    
    def edit_volume(self, args):
        '''
        Edit an existing volume.  The arguments are not all necessary but the user must specify
        something to uniquely identify the volume
        '''
        
        volume = None
        isFromData = False
        
        if ( args["data"] != None):
            isFromData = True
            jsonData = json.loads( args["data"] )
            volume =  VolumeConverter.build_volume_from_json( jsonData )  
        
        if ( isFromData == False and args["volume_name"] != None ):
            volume = self.get_volume_service().find_volume_by_name( args["volume_name"] )
        elif ( isFromData == False and args["volume_id"] != None ):
            volume = self.get_volume_service().find_volume_by_id( args["volume_id"] )
               
        if ( volume.id == None ):
            print "Could not find a volume that matched your entry.\n"
            return       
           
        if ( args["iops_guarantee"] != None and isFromData == False ):
            volume.iops_guarantee = args["iops_guarantee"]
            
        if ( args["iops_limit"] != None and isFromData == False):
            volume.iops_limit = args["iops_limit"]
            
        if ( args["priority"] != None and isFromData == False):
            volume.priority = args["priority"]
            
        if ( args["continuous_protection"] != None and isFromData == False):
            volume.continuous_protection = args["continuous_protection"]      
            
        response = self.get_volume_service().edit_volume( volume );
        
        if ( response != None ):
            self.list_volumes( args )                 
            
    def clone_volume(self, args):
        '''
        Clone a volume from the argument list
        '''
        #now
        fromTime = time.time() * 1000
        volume = None
        
        if ( args["volume_name"] != None):
            volume = self.get_volume_service().find_volume_by_name( args["volume_name"] )
        elif ( args["volume_id" ] != None):
            volume = self.get_volume_service().find_volume_by_id( args["volume_id"] )
        
        if ( args["data"] != None ):
            jsonData = json.loads( args["data"] )
            
    
    def delete_volume(self, args):
        '''
        Delete the indicated volume
        '''
        volName = args["volume_name"]
        
        if ( args["volume_id"] != None):
            volume = self.get_volume_service().find_volume_by_id( args["volume_id"])
            
            if ( volume == None ):
                print "No volume found with an ID of " + args["volume_id"]
                return
                
            volName = volume.name
        
        response = self.get_volume_service().delete_volume( volName )
        
        if ( response["status"] == "OK" ):
            print 'Deletion request completed successfully.'
            
            self.list_volumes(args)
            
    def create_snapshot(self, args):
        '''
        Create a snapshot for a volume
        '''
        
        volume = None
        
        if ( args["volume_name"] != None ):
            volume = self.get_volume_service().find_volume_by_name( args["volume_name"] )
        else:
            volume = self.get_volume_service().find_volume_by_id( args["volume_id"] )
            
        if ( volume == None ):
            print "No volume found with the specified identification.\n"
            return
        
        snapshot = Snapshot()
        
        snapshot.name = args["name"]
        snapshot.retention = args["retention"]
        snapshot.timeline_time = 0
        snapshot.volume_id = volume.id
        
        response = self.get_volume_service().create_snapshot( snapshot )
        
        if ( response != None ):
            self.list_snapshots(args)
         
    def list_snapshots(self, args):
        '''
        List snapshots for this volume
        '''
        volId = args["volume_id"]
        
        if ( args["volume_name"] != None):
            volume = self.get_volume_service().find_volume_by_name( args["volume_name"])
            
            if ( volume == None ):
                print "No volume found with a name of " + args["volume_name"] + "\n"
                return
            
            volId = volume.id
            
        response = self.get_volume_service().list_snapshots(volId)
        
        if ( len( response ) == 0 ):
            print "No snapshots found for volume with ID " + volId;
        
        #print it all out
        if "format" in args  and args["format"] == "json":
            response_writer.ResponseWriter.writeJson( response )
        else:
            #The tabular format is very poor for a volume object, so we need to remove some keys before display
            resultList = []
            
            for snap in response:
                fields = []
                
                snapshot = SnapshotConverter.build_snapshot_from_json( snap )
                created = time.localtime( snapshot.created )
                created = time.strftime( "%c", created )
                
                fields.append( ("ID", snapshot.id))
                fields.append( ("Name", snapshot.name))
                fields.append( ("Created", created))
                
                retentionValue = snapshot.retention
                
                if ( retentionValue == 0 ):
                    retentionValue = "Forever"
                
                fields.append( ("Retention", retentionValue))
                
                ov = OrderedDict( fields )
                resultList.append( ov )   
            #end for loop
            
            response_writer.ResponseWriter.writeTabularData( resultList )  
            
        
    