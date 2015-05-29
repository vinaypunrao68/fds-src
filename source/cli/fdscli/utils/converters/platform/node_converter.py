import json
from model.platform.node import Node
from service_converter import ServiceConverter

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
    def build_node_from_json( jsonString ):
        
        node = Node()
        
        if not isinstance( jsonString, dict ):
            jsonString = json.loads(jsonString)

        node.id = jsonString.pop( "uuid", -1 )
        node.state = jsonString.pop( "state", "UP" )
        node.ip_v4_address = jsonString.pop( "ipV4address", "127.0.0.1" )
        node.ip_v6_address = jsonString.pop( "ipV6address", "" )
        node.name = jsonString.pop( "name", node.ip_v4_address )
        
        services = jsonString.pop( "services", None )
        
        if ( services is not None ):
            for key in ("AM","DM","SM","PM","OM"):
                NodeConverter.create_services_from_json(node, key, services)
        
        return node
    
    @staticmethod
    def to_json( node ):
        d = dict()
        
        d["uuid"] = node.id
        d["state"] = node.state
        d["ipV4address"] = node.ip_v4_address
        d["ipV6address"] = node.ip_v6_address
        d["name"] = node.name
        
        s_dict = dict()
        
        for key in  ("AM","DM","PM","OM","SM"):
            s_dict[key] = NodeConverter.create_service_from_objects(node, key)
        
        d["services"] = s_dict
        
        result = json.dumps( d )
        
        return result;
        
        
