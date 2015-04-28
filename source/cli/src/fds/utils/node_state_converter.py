import json
from fds.model.node_state import NodeState

class NodeStateConverter():
    '''
    Created on Apr 23, 2015
    
    @author: nate
    '''
    
    @staticmethod
    def build_node_state_from_json( jsonString ):
        
        node_state = NodeState()
        node_state.am = jsonString.pop( "am", node_state.am )
        node_state.dm = jsonString.pop( "dm", node_state.dm )
        node_state.sm = jsonString.pop( "sm", node_state.sm )
        
        return node_state
    
    @staticmethod
    def to_json( node_state ):
        d = dict()
        
        d["am"] = node_state.am
        d["dm"] = node_state.dm
        d["sm"] = node_state.sm
        
        result = json.loads( d )
        
        return result
