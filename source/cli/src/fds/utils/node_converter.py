import json
from fds.model.node import Node
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
        
        for service in node.services[key]:
            j_service = ServiceConverter.to_json( service )
            services.append( json.loads(j_service) )
            
        return services
    
    @staticmethod
    def build_node_from_json( jsonString ):
        
        node = Node()

        node.id = jsonString.pop( "uuid", -1 )
        node.state = jsonString.pop( "state", "UP" )
        node.ip_v4_address = jsonString.pop( "ipV4address", "127.0.0.1" )
        node.ip_v6_address = jsonString.pop( "ipV6address", "" )
        node.name = jsonString.pop( "name", node.ip_v4_address )
        
        services = jsonString.pop( "services", None )
        
        if ( services != None ):
            NodeConverter.create_services_from_json(node, "AM", services)
            NodeConverter.create_services_from_json(node, "DM", services)
            NodeConverter.create_services_from_json(node, "SM", services)
            NodeConverter.create_services_from_json(node, "PM", services)
            NodeConverter.create_services_from_json(node, "OM", services)
        
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
        s_dict["AM"] = NodeConverter.create_service_from_objects(node, "AM")
        s_dict["DM"] = NodeConverter.create_service_from_objects(node, "DM")
        s_dict["SM"] = NodeConverter.create_service_from_objects(node, "SM")
        s_dict["OM"] = NodeConverter.create_service_from_objects(node, "OM")
        s_dict["PM"] = NodeConverter.create_service_from_objects(node, "PM")                                          
        
        d["services"] = s_dict
        
        result = json.dumps( d )
        
        return result;
        
        