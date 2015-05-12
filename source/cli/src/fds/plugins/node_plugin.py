from abstract_plugin import AbstractPlugin
from fds.services.node_service import NodeService
from fds.services.response_writer import ResponseWriter
from fds.model.node_state import NodeState
from fds.utils.node_converter import NodeConverter

import json

class NodePlugin( AbstractPlugin ):
    '''
    Created on Apr 27, 2015
    
    @author: nate
    '''
    
    def __init__(self, session):
        AbstractPlugin.__init__(self, session)
        self.__node_service = NodeService( self.session )
        
    '''
    @see: AbstractPlugin
    '''
    def build_parser(self, parentParser, session): 
        
        self.__parser = parentParser.add_parser( "node", help="Interact with node commands" )
        self.__subparser = self.__parser.add_subparsers( help="The sub-commands that are available")
        
        self.create_list_parser( self.__subparser )
        self.create_list_services_parser( self.__subparser )
        self.create_shutdown_parser( self.__subparser )
        self.create_start_parser( self.__subparser )
        self.create_remove_parser( self.__subparser )
        self.create_add_parser( self.__subparser )
        self.create_start_service_parser( self.__subparser )
        self.create_stop_service_parser( self.__subparser )
        
        
    def create_list_parser(self, subparser):    
        '''
        Create the listing commands
        '''
    
        __list_parser = subparser.add_parser( "list", help="Get a list of nodes in the environment")
        __list_parser.add_argument( "-" + AbstractPlugin.format_str, help="Specify the format that the result is printed as", choices=["json","tabular"], required=False )
        __list_parser.add_argument( "-" + AbstractPlugin.state_str, help="Filter the list by a specific state.", choices=["discovered","added","all"], default="all")
     
        __list_parser.set_defaults( func=self.list_nodes, format="tabular")
     
    def create_list_services_parser(self, subparser):
        '''
        A parser that can list the services on a node or nodes
        '''
        
        __service_parser = subparser.add_parser( "list_services", help="List services that are running on a node or nodes.  Default is to see all services on all nodes." )
        __service_parser.add_argument( "-" + AbstractPlugin.format_str, help="Specify the format that the result is printed as", choices=["json","tabular"], required=False )
        __service_parser.add_argument( "-" + AbstractPlugin.node_id_str, help="Specify which node you would like to list the services for.")
        __service_parser.add_argument( "-" + AbstractPlugin.services_str, help="A list of what type of services you would like to see in the results.", nargs="+", choices=["am","dm","sm", "pm", "om"],default=["am","dm","sm","pm","om"],required=False)
         
        __service_parser.set_defaults( func=self.list_services, format="tabular")
     
    def create_shutdown_parser(self, subparser):
        '''
        Create a parser for shutdown
        '''
        
        __shutdown_parser = subparser.add_parser( "shutdown", help="Shutdown a specific node")
        __shutdown_parser.add_argument( "-" + AbstractPlugin.node_id_str, help="The UUID of the node you wish to shutdown.", required=True)
        
        __shutdown_parser.set_defaults( func=self.stop_node, format="tabular" )
        
    def create_start_parser(self, subparser):
        '''
        Create a parser for node start
        '''
        
        __shutdown_parser = subparser.add_parser( "start", help="Start a specific node")
        __shutdown_parser.add_argument( "-" + AbstractPlugin.node_id_str, help="The UUID of the node you wish to start.", required=True)
        
        __shutdown_parser.set_defaults( func=self.start_node, format="tabular" )        
     
    def create_remove_parser(self, subparser):
        '''
        Create a parser for removing a node
        '''
        
        __remove_parser = subparser.add_parser( "remove", help="Remove the specified node from the system")
        __remove_parser.add_argument( "-" + AbstractPlugin.node_id_str, help="The UUID of the node you wish to remove.", required=True)
        
        __remove_parser.set_defaults( func=self.remove_node, format="tabular" ) 
     
    def create_add_parser(self, subparser):
        '''
        Create a parser to activate a node
        '''
        
        __activate_parser = subparser.add_parser( "add", help="Activate a node that is currently in the discovered state. If no arguments are supplied all the nodes will be activated." )
        __activate_parser.add_argument( "-" + AbstractPlugin.format_str, help="Specify the format that the result is printed as", choices=["json","tabular"], required=False )
        __activate_parser.add_argument( "-" + AbstractPlugin.node_ids_str, help="A list of UUIDs for the nodes you would like to activate.", nargs="+")
        
        __activate_parser.set_defaults( func=self.add_nodes, format="tabular") 
     
    def create_start_service_parser(self, subparser):
        '''
        Create a service for starting services
        '''
        
        __start_parser = subparser.add_parser( "start_service", help="Start all, or specific services on a specified node.  If no services are specified it will try to start all services." )
        __start_parser.add_argument( "-" + AbstractPlugin.format_str, help="Specify the format that the result is printed as", choices=["json","tabular"], required=False )        
        __start_parser.add_argument( "-" + AbstractPlugin.node_id_str, help="The UUID of the node on which you'd like to start services", required=True)
        __start_parser.add_argument( "-" + AbstractPlugin.service_id_str, help="The UUID of the service that you would like to start.", required=True)
        
        __start_parser.set_defaults( func=self.start_service, format="tabular")

    def create_stop_service_parser(self, subparser):
        '''
        Create a service for starting services
        '''
        
        __stop_parser = subparser.add_parser( "stop_service", help="Stop all, or specific services on a specified node.  If no services are specified it will try to stop all services." )
        __stop_parser.add_argument( "-" + AbstractPlugin.format_str, help="Specify the format that the result is printed as", choices=["json","tabular"], required=False )        
        __stop_parser.add_argument( "-" + AbstractPlugin.node_id_str, help="The UUID of the node on which you'd like to stop services", required=True)
        __stop_parser.add_argument( "-" + AbstractPlugin.service_id_str, help="The UUID of the service that you would like to stop.", required=True)
     
        __stop_parser.set_defaults( func=self.stop_service, format="tabular")
     
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
    
    def filter_for_discovered_nodes(self, nodes):
        '''
        Takes a list of raw nodes and returns only
        the ones in a discovered state
        '''
        
        d_nodes = []
        
        for node in nodes:
            if ( node.state == "DISCOVERED" ):
                d_nodes.append( node )
    
        return d_nodes
        
    def filter_for_added_nodes(self, nodes):
        '''
        Takes a raw list of nodes and returns everything that
        is not in a discovered state
        '''
        
        d_nodes = []
        
        for node in nodes:
            if ( node.state != "DISCOVERED" ):
                d_nodes.append( node )
    
        return d_nodes


    #handler functions
    def list_nodes(self, args):
        '''
        Execute the list node logic
        
        Since there is no real filtering logic or reasonable set of call yet,
        we have the burden of parsing and deciphering the request here. 
        
        This should be fixed in the next revision of the REST API
        '''
        
        nodes = self.get_node_service().list_nodes()
        show_list = []
        
        if ( AbstractPlugin.state_str in args and args[AbstractPlugin.state_str] == "discovered" ):
            show_list = self.filter_for_discovered_nodes(nodes)
        elif ( AbstractPlugin.state_str in args and args[AbstractPlugin.state_str] == "added" ):
            show_list = self.filter_for_added_nodes(nodes)
        else:
            show_list = nodes
            
        if ( len( show_list ) == 0 ):
            print "\nNo nodes matching the request were found."
            
        if args[AbstractPlugin.format_str] == "json":
            j_nodes = []
            
            for node in nodes:
                j_node = NodeConverter.to_json(node)
                j_node = json.loads( j_node )
                j_nodes.append( j_node )
                
            ResponseWriter.writeJson( j_nodes )
        else:
            cleaned = ResponseWriter.prep_node_for_table( self.session, show_list )
            ResponseWriter.writeTabularData( cleaned )    
                  
    def add_nodes(self, args):
        '''
        Activate a set of discovered nodes
        '''
        id_list = []
        
        if ( args[AbstractPlugin.node_ids_str] is None ):
            nodes = self.filter_for_discovered_nodes( self.get_node_service().list_nodes() )
            for node in nodes:
                id_list.append( node["uuid"] )
            #end of for loop
        else:
            id_list = args[AbstractPlugin.node_ids_str]
            
        # make each call
        state = NodeState()
        
        failures = []
        
        for uuid in id_list:
            result = self.get_node_service().activate_node( uuid, state )
            if ( result["status"] != 200 ):
                failures.append( uuid )
            
        if ( len( failures ) > 0 ):
            print "The following IDs were not activated due to errors: {}\n".format( failures )
            
        self.list_nodes(args)
        
    def remove_node(self, args):
        '''
        Remove a specific node
        '''
        response = self.get_node_service().remove_node(args[AbstractPlugin.node_id_str])
        
        if ( response["status"] == 200 ):
            self.list_nodes(args)
            
    def start_node(self, args):
        '''
        Start all the services on a given node
        '''
        
        response = self.get_node_service().start_node(args[AbstractPlugin.node_id_str])
        
        if ( response["status"] == 200 ):
            self.list_nodes(args)            
            
    def stop_node(self, args):
        '''
        Stop all the services on a given node
        '''
        
        response = self.get_node_service().stop_node(args[AbstractPlugin.node_id_str])
        
        if ( response["status"] == 200 ):
            self.list_nodes(args)
                   
    def start_service(self, args):
        '''
        Start services on a specific node
        '''
            
        response = self.get_node_service().start_service( args[AbstractPlugin.node_id_str], args[AbstractPlugin.service_id_str])
            
        if ( response["status"] == "ok" ):
            self.list_nodes(args)            
                        
    def stop_service(self, args):
        '''
        Stop services on a specific node
        '''
        
        response = self.get_node_service().stop_service( args[AbstractPlugin.node_id_str], args[AbstractPlugin.service_id_str])

        if ( response["status"] == "ok" ):
            self.list_nodes(args)                           
                        
    def list_services(self, args):
        '''
        Handler to figure out which services to list
        '''
        
        nodes = self.get_node_service().list_nodes()
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
                     
        