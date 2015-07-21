import json
from model.platform.node import Node
from utils.converters.platform.service_converter import ServiceConverter
from utils.converters.platform.address_converter import AddressConverter

class NodeConverter():
    '''
    Created on Apr 27, 2015

    class to convert between a node object and the json representation
    
    @author: nate
    '''
    @staticmethod
    def create_services_from_json( node, key, json_services ):
        
        services = json_services.pop( key, [] )
        
        for service in services:
            o_service = ServiceConverter.build_service_from_json( service )
            node.services[key].append( o_service )
    
    @staticmethod
    def create_service_from_objects( node, key):
        
        services = []
        
        if not key in node.services:
            return services
        
        for service in node.services[key]:
            j_service = ServiceConverter.to_json( service )
            services.append( json.loads(j_service) )
            
        return services
    
    @staticmethod
    def build_node_from_json( j_str ):
        
        node = Node()
        
        if not isinstance( j_str, dict ):
            j_str = json.loads(j_str)

        node.id = j_str.pop("uid", node.id)
        node.name = j_str.pop("name", node.name)
        node.state = j_str.pop( "state", "UP" )
        node.address = AddressConverter.build_address_from_json(j_str.pop("address", node.address))
        
        services = j_str.pop( "serviceMap", None )
        
        if ( services is not None ):
            for key in ("AM","DM","SM","PM","OM"):
                NodeConverter.create_services_from_json(node, key, services)
        
        return node
    
    @staticmethod
    def to_json( node ):
        d = dict()
        
        d["uid"] = node.id
        d["name"] = node.name
        d["state"] = node.state
        
        d["address"] = json.loads(AddressConverter.to_json(node.address))
        
        s_dict = dict()
        
        for key in  ("AM","DM","PM","OM","SM"):
            s_dict[key] = NodeConverter.create_service_from_objects(node, key)
        
        d["serviceMap"] = s_dict
        
        result = json.dumps( d )
        
        return result;
        
        
