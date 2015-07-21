from base_cli_test import BaseCliTest
import mock_functions
from mock import patch
from utils.converters.platform.node_converter import NodeConverter

class TestServices(BaseCliTest):
    '''
    Created on May 13, 2015
    
    @author: nate
    '''
    @patch( "services.node_service.NodeService.start_service", side_effect=mock_functions.startService)
    @patch( "services.node_service.NodeService.list_nodes", side_effect=mock_functions.listNodes)
    def test_start_service(self, mockList, mockStart):
        '''
        Test trying to start a specific service
        '''
        
        #no IDs
        args = ["service", "start"]
        
        self.callMessageFormatter(args)
        self.cli.run(args)
        
        assert mockStart.call_count == 0
        
        args.append( "-node_id=21ABC" )
        
        self.callMessageFormatter(args)
        self.cli.run(args)
        
        assert mockStart.call_count == 0
        
        args.append( "-service_id=1" )
        
        self.callMessageFormatter(args)
        self.cli.run(args)
        
        assert mockStart.call_count == 1
        assert mockList.call_count == 1
        
        node_id = mockStart.call_args[0][0]
        service_id = mockStart.call_args[0][1]
        
        assert node_id == "21ABC"
        assert service_id == "1"
        
    @patch( "services.node_service.NodeService.stop_service", side_effect=mock_functions.stopService)
    @patch( "services.node_service.NodeService.list_nodes", side_effect=mock_functions.listNodes)
    def test_stop_service(self, mockList, mockStop):
        '''
        Test trying to start a specific service
        '''
        
        #no IDs
        args = ["service", "stop"]
        
        self.callMessageFormatter(args)
        self.cli.run(args)
        
        assert mockStop.call_count == 0
        
        args.append( "-node_id=21ABC" )
        
        self.callMessageFormatter(args)
        self.cli.run(args)
        
        assert mockStop.call_count == 0
        
        args.append( "-service_id=1" )
        
        self.callMessageFormatter(args)
        self.cli.run(args)
        
        assert mockStop.call_count == 1
        assert mockList.call_count == 1
        
        node_id = mockStop.call_args[0][0]
        service_id = mockStop.call_args[0][1]
        
        assert node_id == "21ABC"
        assert service_id == "1"   
        
    @patch( "services.node_service.NodeService.remove_service", side_effect=mock_functions.removeService)        
    @patch( "services.node_service.NodeService.list_nodes", side_effect=mock_functions.listNodes)
    def test_remove_service(self, mockList, mockRemove):
        '''
        Test the removal of a specific service
        '''
        #no ids
        args = ["service", "remove"]
        
        self.callMessageFormatter(args)
        self.cli.run(args)
        
        assert mockRemove.call_count == 0
        
        args.append( "-node_id=21ABC" )
        self.callMessageFormatter(args)
        self.cli.run(args)     
        
        assert mockRemove.call_count == 0
        
        args.append( "-service_id=1" )
        self.callMessageFormatter(args)
        self.cli.run(args)
        
        assert mockRemove.call_count == 1
        assert mockList.call_count == 1
        
        node_id = mockRemove.call_args[0][0]
        service_id = mockRemove.call_args[0][1]
        
        assert node_id == "21ABC"
        assert service_id == "1"
        
    @patch( "services.node_service.NodeService.add_service", side_effect=mock_functions.addService)        
    @patch( "services.node_service.NodeService.list_nodes", side_effect=mock_functions.listNodes)
    def test_add_service(self, mockList, mockAdd):
        '''
        Test the removal of a specific service
        '''
        #no ids
        args = ["service", "add"]
        
        self.callMessageFormatter(args)
        self.cli.run(args)
        
        assert mockAdd.call_count == 0
        
        args.append( "-node_id=21ABC" )
        self.callMessageFormatter(args)
        self.cli.run(args)     
        
        assert mockAdd.call_count == 0
        
        args.append( "-service=dm" )
        self.callMessageFormatter(args)
        self.cli.run(args)
        
        assert mockAdd.call_count == 1
        assert mockList.call_count == 1
        
        node_id = mockAdd.call_args[0][0]
        service = mockAdd.call_args[0][1]
        
        assert node_id == "21ABC"
        assert service.type == "DM"
        assert service.name == "DM"    
        
        args[3] = "-service=am"
        self.callMessageFormatter(args)
        self.cli.run(args)
        
        assert mockAdd.call_count == 2
        assert mockList.call_count == 2
        
        node_id = mockAdd.call_args[0][0]
        service = mockAdd.call_args[0][1]
        
        assert node_id == "21ABC"
        assert service.type == "AM"
        assert service.name == "AM"   
        
        args[3] = "-service=sm"
        self.callMessageFormatter(args)
        self.cli.run(args)
        
        assert mockAdd.call_count == 3
        assert mockList.call_count == 3
        
        node_id = mockAdd.call_args[0][0]
        service = mockAdd.call_args[0][1]
        
        assert node_id == "21ABC"
        assert service.type == "SM"
        assert service.name == "SM"

    @patch( "services.response_writer.ResponseWriter.writeJson", side_effect=mock_functions.writeJson)
    @patch( "services.node_service.NodeService.list_nodes", side_effect=mock_functions.listNodes)
    def test_list_services(self, mockList, mockWriteJson):
        '''
        Test the list services functionality.  This one is a little more intense because
        there isn't a list services ability in the REST endpoints yet, so the plugin function
        really just plays games with the results from list nodes in order to filter the results.
        
        Therefore, we need to inspect the objects sent to the writer directly.
        '''
        
        #all services
        args = ["service", "list", "-format=json"]
    
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
        args = ["service", "list", "-services", "sm", "dm", "-format=json"]
    
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
        args = ["service", "list", "-services", "am", "pm", "-format=json"]
    
        self.callMessageFormatter(args)
        self.cli.run(args)
        
        assert mockWriteJson.call_count == 3
        
        j_node = mockWriteJson.call_args[0][0]
        node = NodeConverter.build_node_from_json( j_node[0] )
        
        assert len( node.services["AM"] ) == 1
        assert len( node.services["PM"] ) == 1
        assert len( node.services["SM"] ) == 0
        assert len( node.services["DM"] ) == 0        
