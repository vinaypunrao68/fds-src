from fds.services import volume_service
from fds.services import response_writer
from fds.model.volume import Volume
from fds.utils.volume_converter import VolumeConverter
from collections import OrderedDict

import abstract_plugin
import json
import time
from fds.utils.volume_validator import VolumeValidator

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
        
        if ( self.session.isAllowed( "VOL_MGMT" ) == False ):
            return
        
        self.__volume_service = volume_service.VolumeService( session )
        
        self.__parser = parentParser.add_parser( "volume", help="All volume management operations" )
        self.__subparser = self.__parser.add_subparsers( help="The sub-commands that are available")
        
        self.createListCommand( self.__subparser )
        self.createCreateCommand( self.__subparser )
        

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
            
            if ( self.session.isAllowed( "TENANT_MGMT" ) ):
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
            
        return cleanedResponses
            

    def success(self, response, args ):
        '''
        Default success handler to determine the print options
        '''        
        if "format" in args  and args["format"] == "json":
            response_writer.ResponseWriter.writeJson( response )
        else:
            #The tabular format is very poor for a volume object, so we need to remove some keys before display
            response = self.clean_response( response )
            response_writer.ResponseWriter.writeTabularData( response )        
        
#parser creation        
        
 
    def createListCommand(self, subparser ):
        
        '''
        Configure the parser for the volume list command
        '''           
        __listParser = subparser.add_parser( "list", help="List all the volumes in the system" )
        __listParser.add_argument( "-format", help="Specify the format that the result is printed as", choices=["json","tabular"], required=False )
        
        indGroup = __listParser.add_argument_group( "Individual volume queries", "Indicate how to identify the volume that you are looking for." )
        __listParserGroup = indGroup.add_mutually_exclusive_group()
        __listParserGroup.add_argument( "-volume_id", help="Specify a particular volume to list by UUID" )
        __listParserGroup.add_argument( "-volume_name", help="Specify a particular volume to list by name" )
        __listParser.set_defaults( func=self.listVolumes, format="tabular" )
        

    def createCreateCommand(self, subparser):

        '''
        Create the parser for the volume creation command
        '''
                 
        __createParser = subparser.add_parser( "create", help="Create a new volume" )
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
        
        __createParser.set_defaults( func=self.createVolume )
        
# Area for actual calls and JSON manipulation        
         
    def listVolumes(self, args):
        '''
        Retrieve a list of volumes in accordance with the passed in arguments
        '''
        response = self.get_volume_service().listVolumes()
        
        self.success( response, args )
        

    def createVolume(self, args):
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
        
        volume = self.get_volume_service().createVolume( volume )
        
        if ( volume != None ):
            self.listVolumes(args)
            
        return
    