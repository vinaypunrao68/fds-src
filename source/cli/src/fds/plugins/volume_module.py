'''
Created on Apr 3, 2015

@author: nate
'''
from fds.services import volume_service
from fds.services import response_writer
from fds.model.volume import Volume
import abstract_plugin
from collections import OrderedDict

class VolumeModule( abstract_plugin.AbstractPlugin):
    
    def build_parser(self, parentParser, session): 
        
        self.__volume_service = volume_service.VolumeService( session )
        
        self.__parser = parentParser.add_parser( "volume", help="All volume management operations" )
        self.__subparser = self.__parser.add_subparsers( help="The sub-commands that are available")
        
        self.createListCommand( self.__subparser )
        self.createCreateCommand( self.__subparser )
        
    def detect_shortcut(self, args):

        # is it the listVolumes shortcut
        if ( args[0].endswith( "listVolumes" ) ):
            args.pop(0)
            args.insert( 0, "volume" )
            args.insert( 1, "list" )
            return args
        
        return None
        
    def get_volume_service(self):
        return self.__volume_service  
    
    def clean_response(self, response):
        
        cleanedResponses = []
        
        for jVolume in response:
            volume = Volume.build_volume_from_json( jVolume )
            ov = OrderedDict( [("ID", volume.get_id()),
                               ("Name", volume.get_name()),
                               ("Type", volume.get_type()),
                               ("Priority", volume.get_priority()),
                               ("State", volume.get_state()),
                               ("Usage", volume.get_current_size() + " " + volume.get_current_units())])
            cleanedResponses.append( ov )
            
        return cleanedResponses
            
    
    def success(self, response, args ):
        if args["format"] == "json":
            response_writer.ResponseWriter.writeJson( response )
        else:
            #The tabular format is very poor for a volume object, so we need to remove some keys before display
            response = self.clean_response( response )
            response_writer.ResponseWriter.writeTabularData( response )        
        
#parser creation        
        
    def createListCommand(self, subparser ):
        __listParser = subparser.add_parser( "list", help="List all the volumes in the system" )
        __listParser.add_argument( "-format", help="Specify the format that the result is printed as", choices=["json","tabular"], required=False )
        
        indGroup = __listParser.add_argument_group( "Individual volume queries", "Indicate how to identify the volume that you are looking for." )
        __listParserGroup = indGroup.add_mutually_exclusive_group()
        __listParserGroup.add_argument( "-volume_id", help="Specify a particular volume to list by UUID" )
        __listParserGroup.add_argument( "-volume_name", help="Specify a particular volume to list by name" )
        __listParser.set_defaults( func=self.listVolumes, format="tabular" )
        
    def createCreateCommand(self, subparser):
        __createParser = subparser.add_parser( "create", help="Create a new volume" )
        __createParser.add_argument( "-data", help="JSON string representing the volume parameters desired for the new volume.  This argument will take precedence over all individual arguments.", default=None)
        __createParser.add_argument( "-name", help="The name of the volume", default=None, required=True )
        __createParser.add_argument( "-iops_limit", help="The IOPs limit for the volume.  0 = unlimited and is the default if not specified.", type=int, default=0 )
        __createParser.add_argument( "-iops_guarantee", help="The IOPs guarantee for this volume.  0 = no guarantee and is the default if not specified.", type=int, default=0 )
        __createParser.add_argument( "-priority", help="A value that indicates how to prioritize performance for this volume.  1 = highest priority, 10 = lowest.  Default value is 7.", type=int, choices=range( 1, 10 ), default=7)
        __createParser.add_argument( "-type", help="The type of volume connector to use for this volume.", choices=["object", "block"], default="object")
        __createParser.add_argument( "-media_policy", help="The policy that will determine where the data will live over time.", choices=["hybrid", "ssd_only", "hdd_only"], default="hybrid")
        __createParser.add_argument( "-continuous_protection", help="A value (in seconds) for how long you want continuous rollback for this volume.  All values less than 24 hours will be set to 24 hours.", type=int, default=60400 )
        
        __createParser.set_defaults( func=self.createVolume )
        
# Area for actual calls and JSON manipulation        
        
    def listVolumes(self, args):
        response = self.get_volume_service().listVolumes()
        
        self.success( response, args )
        
    def createVolume(self, args):
        print '{}'.format( args )
        return
    