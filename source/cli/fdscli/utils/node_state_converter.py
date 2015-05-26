import json
from model.node_state import NodeState

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
        
        if ( node_state.am is not None ):
            d["am"] = node_state.am
            
        if ( node_state.dm is not None ):
            d["dm"] = node_state.dm
            
        if ( node_state.sm is not None ):
            d["sm"] = node_state.sm
        
        result = json.dumps( d )
        
        return result
