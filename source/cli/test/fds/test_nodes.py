from base_cli_test import BaseCliTest
import mock_functions
from mock import patch
from __builtin__ import True
from fds.utils.node_converter import NodeConverter

def filter_for_discovered(nodes):
    return nodes

def filter_for_added(nodes):
    return nodes

def writeJson(data):
    return

class TestNodes( BaseCliTest ):
    '''
    Created on Apr 27, 2015
    
    @author: nate
    '''
    
    @patch( "fds.plugins.node_plugin.NodePlugin.filter_for_added_nodes", side_effect=filter_for_added )
    @patch( "fds.plugins.node_plugin.NodePlugin.filter_for_discovered_nodes", side_effect=filter_for_discovered )
    @patch( "fds.services.node_service.NodeService.list_nodes", side_effect=mock_functions.listNodes)
    def test_list_nodes(self, mockList, mockDisc, mockAdd):
        '''
        Test that list nodes gets called correctly with the args
        '''
        
        args = ["node", "list"]
        self.callMessageFormatter(args)
        self.cli.run(args)
        
        assert mockList.call_count == 1
        assert mockDisc.call_count == 0
        assert mockAdd.call_count == 0
        
        args = ["node", "list", "-state=discovered"]
        self.callMessageFormatter(args)
        self.cli.run( args )
        
        assert mockList.call_count == 2
        assert mockDisc.call_count == 1
        assert mockAdd.call_count == 0
        
        args = ["node", "list", "-state=added"]
        self.callMessageFormatter(args)
        self.cli.run( args )
        
        assert mockList.call_count == 3
        assert mockDisc.call_count == 1
        assert mockAdd.call_count == 1
        
    @patch( "fds.services.node_service.NodeService.activate_node", side_effect=mock_functions.activateNode)
    @patch( "fds.services.node_service.NodeService.list_nodes", side_effect=mock_functions.listNodes)
    def test_activate_node(self, mockList, mockActivate):
        '''
        Test that activate is called with all three services set to true
        '''
        
        #failure call first
        args = ["node", "activate"]
        self.callMessageFormatter(args)
        self.cli.run(args)
         
        assert mockActivate.call_count == 0
        
        args = ["node", "activate", "-node_ids", "1", "2", "3"]
        self.callMessageFormatter(args)
        self.cli.run(args)
        
        assert mockActivate.call_count == 3
        
        first = mockActivate.call_args_list[0]
        second = mockActivate.call_args_list[1]
        third = mockActivate.call_args_list[2]
        
        assert first[0][0] == "1"
        assert second[0][0] == "2"
        assert third[0][0] == "3"
        
        assert first[0][1].am == second[0][1].am == third[0][1].am
        assert first[0][1].dm == second[0][1].dm == third[0][1].dm
        assert first[0][1].sm == second[0][1].sm == third[0][1].sm
        
    @patch( "fds.services.node_service.NodeService.deactivate_node", side_effect=mock_functions.deactivateNode)
    @patch( "fds.services.node_service.NodeService.list_nodes", side_effect=mock_functions.listNodes)
    def test_deactivate_node(self, mockList, mockDeactivate):
        '''
        Test that activate is called with all three services set to true
        '''
        
        #failure call first
        args = ["node", "activate"]
        self.callMessageFormatter(args)
        self.cli.run(args)
         
        assert mockDeactivate.call_count == 0
        
        args = ["node", "deactivate", "-node_id=1"]
        self.callMessageFormatter(args)
        self.cli.run(args)
        
        assert mockDeactivate.call_count == 1
        
        first = mockDeactivate.call_args_list[0]
        
        assert first[0][0] == "1"
        
        assert first[0][1].am == False
        assert first[0][1].dm == False
        assert first[0][1].sm == False
        
    @patch( "fds.services.node_service.NodeService.activate_node", side_effect=mock_functions.activateNode)
    @patch( "fds.services.node_service.NodeService.list_nodes", side_effect=mock_functions.listNodes)        
    def test_start_service(self, mockList, mockActivate):
        '''
        Test the start service call in a bunch of permutations
        '''
        
        # No node ID failure case first
        args = ["node", "start_service"]
        self.callMessageFormatter(args)
        self.cli.run(args)
        
        assert mockActivate.call_count == 0    
        
        #test the default start all services
        args = ["node", "start_service", "-node_id=4"]    
        self.callMessageFormatter(args)
        self.cli.run(args)
    
        assert mockActivate.call_count == 1
        
        node_id = mockActivate.call_args[0][0]
        state = mockActivate.call_args[0][1]
        
        assert node_id == "4"
        assert state.am == True
        assert state.dm == True
        assert state.sm == True
    
        #test only certain services
        args = ["node", "start_service", "-node_id=4", "-services", "am", "dm"]
        
        self.callMessageFormatter(args)
        self.cli.run(args)
        
        assert mockActivate.call_count == 2
        
        state = mockActivate.call_args[0][1]
        
        assert state.am == True
        assert state.dm == True
        assert state.sm == None
        
        args = ["node", "start_service", "-node_id=4", "-services", "sm"]
        
        self.callMessageFormatter(args)
        self.cli.run(args)
        
        assert mockActivate.call_count == 3
        
        state = mockActivate.call_args[0][1]
        
        assert state.am == None
        assert state.dm == None
        assert state.sm == True
    
    @patch( "fds.services.node_service.NodeService.activate_node", side_effect=mock_functions.activateNode)
    @patch( "fds.services.node_service.NodeService.list_nodes", side_effect=mock_functions.listNodes)        
    def test_stop_service(self, mockList, mockActivate):
        '''
        Test the stop service call in a bunch of permutations
        '''
        
        # No node ID failure case first
        args = ["node", "stop_service"]
        self.callMessageFormatter(args)
        self.cli.run(args)
        
        assert mockActivate.call_count == 0    
        
        #test the default start all services
        args = ["node", "stop_service", "-node_id=4"]    
        self.callMessageFormatter(args)
        self.cli.run(args)
    
        assert mockActivate.call_count == 1
        
        node_id = mockActivate.call_args[0][0]
        state = mockActivate.call_args[0][1]
        
        assert node_id == "4"
        assert state.am == False
        assert state.dm == False
        assert state.sm == False
    
        #test only certain services
        args = ["node", "stop_service", "-node_id=4", "-services", "am", "dm"]
        
        self.callMessageFormatter(args)
        self.cli.run(args)
        
        assert mockActivate.call_count == 2
        
        state = mockActivate.call_args[0][1]
        
        assert state.am == False
        assert state.dm == False
        assert state.sm == None
        
        args = ["node", "stop_service", "-node_id=4", "-services", "sm"]
        
        self.callMessageFormatter(args)
        self.cli.run(args)
        
        assert mockActivate.call_count == 3
        
        state = mockActivate.call_args[0][1]
        
        assert state.am == None
        assert state.dm == None
        assert state.sm == False    
    
    @patch( "fds.services.node_service.NodeService.list_nodes", side_effect=mock_functions.listNodes) 
    @patch( "fds.services.response_writer.ResponseWriter.writeJson", side_effect=writeJson)
    def test_list_services(self, mockWriteJson, mockList):
        '''
        Test the list services functionality.  This one is a little more intense because
        there isn't a list services ability in the REST endpoints yet, so the plugin function
        really just plays games with the results from list nodes in order to filter the results.
        
        Therefore, we need to inspect the objects sent to the writer directly.
        '''
        
        #all services
        args = ["node", "list_services", "-format=json"]
    
        self.callMessageFormatter(args)
        self.cli.run(args)
        
        assert mockWriteJson.call_count == 1
        
        j_node = mockWriteJson.call_args[0][0]
        node = NodeConverter.build_node_from_json( j_node[0] )
        
        assert len( node.services["AM"] ) == 1
        assert len( node.services["PM"] ) == 1
        assert len( node.services["SM"] ) == 1
        assert len( node.services["DM"] ) == 1
        
        # just sm and dm
        args = ["node", "list_services", "-services", "sm", "dm", "-format=json"]
    
        self.callMessageFormatter(args)
        self.cli.run(args)
        
        assert mockWriteJson.call_count == 2
        
        j_node = mockWriteJson.call_args[0][0]
        node = NodeConverter.build_node_from_json( j_node[0] )
        
        assert len( node.services["AM"] ) == 0
        assert len( node.services["PM"] ) == 0
        assert len( node.services["SM"] ) == 1
        assert len( node.services["DM"] ) == 1
        
        # just am and pm        
        args = ["node", "list_services", "-services", "am", "pm", "-format=json"]
    
        self.callMessageFormatter(args)
        self.cli.run(args)
        
        assert mockWriteJson.call_count == 3
        
        j_node = mockWriteJson.call_args[0][0]
        node = NodeConverter.build_node_from_json( j_node[0] )
        
        assert len( node.services["AM"] ) == 1
        assert len( node.services["PM"] ) == 1
        assert len( node.services["SM"] ) == 0
        assert len( node.services["DM"] ) == 0    
    
    
    
        
        