from abstract_service import AbstractService
from fds.utils.node_state_converter import NodeStateConverter
from fds.utils.node_converter import NodeConverter
from fds.model.node_state import NodeState

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
        
        url = "{}{}".format( self.get_url_preamble(), "/api/config/services" )
        j_nodes = self.rest_helper.get( self.session, url )
        j_nodes = j_nodes.pop( "nodes", [] )
        nodes = []
        
        for j_node in j_nodes:
            node = NodeConverter.build_node_from_json( j_node )
            nodes.append( node )
            
        return nodes
    
    def activate_node(self, node_id, node_state):
        '''
        This method will activate a node and put the services in the desired state.
        
        node_id is the UUID of the node to activate
        node_state is a node state object defines which services will be started
        '''
        
        url = "{}{}{}".format( self.get_url_preamble(), "/api/config/services/", node_id)
        data = NodeStateConverter.to_json( node_state )
        return self.rest_helper.post( self.session, url, data )
    
    def deactivate_node(self, node_id):
        '''
        This method will deactivate the node and remember the state that is sent in
        
        node_id is the UUID of the node to de-activate
        node_state is a node state object defines which services will be stopped        
        '''
        node_state = NodeState()
        
        url = "{}{}{}".format( self.get_url_preamble(), "/api/config/services/", node_id )
        data = NodeStateConverter.to_json( node_state )
        return self.rest_helper.put( self.session, url, data )
        