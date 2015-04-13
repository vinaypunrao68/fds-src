'''
Created on Apr 3, 2015

@author: nate
'''
from fds.services import volume_service
from fds.services import response_writer
import abstract_plugin

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
    
    def success(self, response, args ):
        if args["format"] == "json":
            response_writer.ResponseWriter.writeJson( response )
        else:
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
        __createParser.add_argument( "-name", help="The name of the volume", default=None )
        
        __createParser.set_defaults( func=self.createVolume )
        
# Area for actual calls and JSON manipulation        
        
    def listVolumes(self, args):
        response = self.get_volume_service().listVolumes()
        self.success( response, args )
        
    def createVolume(self, args):
        print '{}'.format( args )
        return
    
    def createVolumeFromData(self, args ):
        print '{}'.format( args  )
        return
    