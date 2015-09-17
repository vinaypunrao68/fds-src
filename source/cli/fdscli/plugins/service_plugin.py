from .abstract_plugin import AbstractPlugin

from services.node_service import NodeService
from utils.converters.platform.node_converter import NodeConverter
from services.response_writer import ResponseWriter
from model.platform.service import Service
from model.platform.service_status import ServiceStatus

import json
from model.fds_error import FdsError
from services.fds_auth import FdsAuth


class ServicePlugin( AbstractPlugin ):
    '''
    Created on Apr 27, 2015
    
    @author: nate
    '''
    
    def __init__(self):
        AbstractPlugin.__init__(self)
        
    '''
    @see: AbstractPlugin
    '''
    def build_parser(self, parentParser, session): 
        
        self.session = session
        
        if not session.is_allowed( FdsAuth.SYS_MGMT ):
            return
        
        self.__node_service = NodeService( self.session )        
        
        self.__parser = parentParser.add_parser( "service", help="Interact with service commands" )
        self.__subparser = self.__parser.add_subparsers( help="The sub-commands that are available")
        
        self.create_list_parser( self.__subparser )
        self.create_remove_parser( self.__subparser )
        self.create_add_parser( self.__subparser )
        self.create_start_parser( self.__subparser )
        self.create_stop_parser( self.__subparser )
        
    '''
    @see: AbstractPlugin
    '''   
    def detect_shortcut(self, args):
        
        return None        

    def get_node_service(self):
        '''
        Re-use the same node service for everything
        '''
        
        return self.__node_service        

# Parsers
    def create_list_parser(self, subparser):
        '''
        parser for the listing of services
        '''
        __list_parser = subparser.add_parser( "list", help="Get a list of services in the system.")
        self.add_format_arg( __list_parser )
        __list_parser.add_argument( "-" + AbstractPlugin.services_str, help="A list of what type of services you would like to see in the results.", nargs="+", choices=["am","dm","sm", "pm", "om"],default=["am","dm","sm","pm","om"],required=False)
        __list_parser.add_argument( "-" + AbstractPlugin.node_id_str, help="The UUID of the node for which you'd like to see services", required=False)
        
        __list_parser.set_defaults( func=self.list_services, format="tabular")
        
    def create_start_parser(self, subparser):
        '''
        Create a service for starting services
        '''
        
        __start_parser = subparser.add_parser( "start", help="Start all, or specific services on a specified node.  If no services are specified it will try to start all services." )
        self.add_format_arg( __start_parser )        
        __start_parser.add_argument( "-" + AbstractPlugin.node_id_str, help="The UUID of the node on which you'd like to start services", required=True)
        __start_parser.add_argument( "-" + AbstractPlugin.service_id_str, help="The UUID of the service that you would like to start.", required=True)
        
        __start_parser.set_defaults( func=self.start_service, format="tabular")

    def create_stop_parser(self, subparser):
        '''
        Create a service for starting services
        '''
        
        __stop_parser = subparser.add_parser( "stop", help="Stop all, or specific services on a specified node.  If no services are specified it will try to stop all services." )
        self.add_format_arg( __stop_parser )        
        __stop_parser.add_argument( "-" + AbstractPlugin.node_id_str, help="The UUID of the node on which you'd like to stop services", required=True)
        __stop_parser.add_argument( "-" + AbstractPlugin.service_id_str, help="The UUID of the service that you would like to stop.", required=True)
     
        __stop_parser.set_defaults( func=self.stop_service, format="tabular")
     
    def create_add_parser(self, subparser):
        '''
        Create a parser to handle the add service request
        '''
        
        __add_service_parser = subparser.add_parser( "add", help="Add a new service to the specified node.")
        self.add_format_arg( __add_service_parser )
        __add_service_parser.add_argument( "-" + AbstractPlugin.node_id_str, help="The UUID of the node you'd like to add the new service to.", required=True)
        __add_service_parser.add_argument( "-" + AbstractPlugin.service_str, help="The type of service you would like to add", choices=["am","dm", "sm"], required=True)
    
        __add_service_parser.set_defaults( func=self.add_service, format="tabular")
        
    def create_remove_parser(self, subparser):
        '''
        Create a parser for handling remove service from node
        '''
        
        __remove_service_parser = subparser.add_parser( "remove", help="Remove a service from a specified node")
        self.add_format_arg( __remove_service_parser )
        __remove_service_parser.add_argument( "-" + AbstractPlugin.node_id_str, help="The UUID of the node that you would like to remove the service from", required=True)
        __remove_service_parser.add_argument( "-" + AbstractPlugin.service_id_str, help="The UUID of the service that you would like to remove", required=True)
        
        __remove_service_parser.set_defaults( func=self.remove_service, format="tabular" )        
        
# actual logic to make correct calls

    def start_service(self, args):
        '''
        Start services on a specific node
        '''
            
        response = self.get_node_service().start_service( args[AbstractPlugin.node_id_str], args[AbstractPlugin.service_id_str])
            
        if not isinstance( response, FdsError ):
            self.list_services(args)            
                        
    def stop_service(self, args):
        '''
        Stop services on a specific node
        '''
        
        response = self.get_node_service().stop_service( args[AbstractPlugin.node_id_str], args[AbstractPlugin.service_id_str])

        if not isinstance( response, FdsError ):
            self.list_services(args)  

    def add_service(self, args):
        ''' 
        use the arguments to make the correct add service call
        '''
        s_type = args[AbstractPlugin.service_str]
        name = s_type.upper()
            
        service = Service( name=name, a_type=name, status=ServiceStatus(state="NOT_RUNNING") )
        
        response = self.get_node_service().add_service( args[AbstractPlugin.node_id_str], service )
        
        if not isinstance( response, FdsError ):
            self.list_services(args)
        
    def remove_service(self, args):
        '''
        Use the args to call remove service properly
        '''
        response = self.get_node_service().remove_service(args[AbstractPlugin.node_id_str], args[AbstractPlugin.service_id_str])
        
        if not isinstance( response, FdsError ):
            self.list_services(args)

    def list_services(self, args):
        '''
        List the correct services from the arguments passed in
        '''
        
        nodes = self.get_node_service().list_nodes()
        
        if isinstance( nodes, FdsError ):
            return
        
        if nodes is None:
            return
        
        node_list = []
        
        # filter everything but the node we want
        if ( args[AbstractPlugin.node_id_str] is not None ):
            for node in nodes:
                if ( node.id == args[AbstractPlugin.node_id_str]):
                    node_list.append( node )
            # end of For
        else:
            node_list = nodes
            
        #OM is not part of the object yet but when it is,
        # these commented out portions will be useful
        
        am = False
        sm = False
        dm = False
        pm = False
#             om = False
        
        # in case we list services from somewhere else where this field is not required
        if AbstractPlugin.services_str not in args:
            am = True
            sm = True
            dm = True
            pm = True
#             om = True
        else:  
            for service in args[AbstractPlugin.services_str]:
                if ( service == "am" ):
                    am = True
                elif ( service == "dm" ):
                    dm = True
                elif ( service == "sm" ):
                    sm = True
                elif ( service == "pm" ):
                    pm = True
    #                 elif ( service == "om" ):
    #                     om = True
            #end of for loop
        
        for node in node_list:
            
            if ( am is False ):
                del node.services["AM"]
                
            if ( sm is False ):
                del node.services["SM"]
                
            if ( dm is False ):
                del node.services["DM"]
                
            if ( pm is False ):
                del node.services["PM"]
                
#                 if ( om == False ):
#                     del node["services"]["OM"]
        #end of for loop
            
        # now we have a list of matching nodes with only the services filled in that are requested
        # if JSON is requested we just spit it out
        if ( args[AbstractPlugin.format_str] == "json" ):
            
            j_nodes = []
            
            for j_node in node_list:
                j_node = NodeConverter.to_json(j_node)
                j_node = json.loads( j_node )
                j_nodes.append( j_node )
                
            ResponseWriter.writeJson( j_nodes )
        else:
            cleaned = ResponseWriter.prep_services_for_table( self.session, node_list )
            ResponseWriter.writeTabularData( cleaned )        
