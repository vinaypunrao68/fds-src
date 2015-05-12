from abstract_service import AbstractService
from fds.utils.node_state_converter import NodeStateConverter
from fds.utils.node_converter import NodeConverter
from fds.model.node import Node
from fds.model.service import Service
from fds.utils.service_converter import ServiceConverter

class NodeService( AbstractService ):
    '''
    Created on Apr 23, 2015
    
    @author: nate
    '''

    def __init__(self, session):
        AbstractService.__init__(self, session)
        
    def list_nodes(self):
        '''
        Get a list of nodes known to the system.  This will include active attached nodes as well as nodes
        that are in the "DISCOVERED" State
        
        The nodes will also have all of their services and states within the returned object
        '''
        
        url = "{}{}".format( self.get_url_preamble(), "/api/config/nodes" )
        j_nodes = self.rest_helper.get( self.session, url )
        j_nodes = j_nodes.pop( "nodes", [] )
        nodes = []
        
        for j_node in j_nodes:
            node = NodeConverter.build_node_from_json( j_node )
            nodes.append( node )
            
        return nodes
    
    def add_node(self, node_id, node_state):
        '''
        This method will activate a node and put the services in the desired state.
        
        node_id is the UUID of the node to activate
        node_state is a node state object defines which services will be started
        '''
        
        url = "{}{}{}".format( self.get_url_preamble(), "/api/config/nodes/", node_id, "/1" )
        data = NodeStateConverter.to_json( node_state )
        return self.rest_helper.post( self.session, url, data )
    
    def start_node(self, node_id):
        '''
        This method will try to put a node into an "UP" state.  Under the covers
        this will tell the system to turn on all of the known services on that node
        '''
        node = Node(an_id=node_id, state="UP",name=node_id)
        url = "{}{}{}".format( self.get_url_preamble(), "/api/config/nodes/", node_id )
        data = NodeConverter.to_json(node)
        return self.rest_helper.put( self.session, url, data )
    
    def stop_node(self, node_id):
        '''
        This method will try to put a node into an "DOWN" state.  Under the covers
        this will tell the system to turn off all of the known services on that node
        '''
        node = Node(an_id=node_id, state="DOWN",name=node_id)
        url = "{}{}{}".format( self.get_url_preamble(), "/api/config/nodes/", node_id )
        data = NodeConverter.to_json(node)
        return self.rest_helper.put( self.session, url, data )    
    
    def remove_node(self, node_id, node_state):
        '''
        This method will deactivate the node and remember the state that is sent in
        
        node_id is the UUID of the node to de-activate
        node_state is a node state object defines which services will be stopped        
        '''
        
        url = "{}{}{}".format( self.get_url_preamble(), "/api/config/nodes/", node_id )
        data = NodeStateConverter.to_json( node_state )
        return self.rest_helper.put( self.session, url, data )
    
    def start_service(self, node_id, service_id):
        '''
        Start a specific service on a node.
        '''
        service = Service(an_id=service_id,status="ACTIVE")
        url = "{}{}{}{}{}".format( self.get_url_preamble(), "/api/config/nodes/", node_id, "/services/", service_id)
        data = ServiceConverter.to_json(service)
        return self.rest_helper.put( self.session, url, data )
    
    def stop_service(self, node_id, service_id):
        '''
        Start a specific service on a node.
        '''
        service = Service(an_id=service_id,status="INACTIVE")
        url = "{}{}{}{}{}".format( self.get_url_preamble(), "/api/config/nodes/", node_id, "/services/", service_id)
        data = ServiceConverter.to_json(service)
        return self.rest_helper.put( self.session, url, data )    
        