from base_cli_test import BaseCliTest
import mock_functions
from mock import patch
from utils.converters.volume.volume_converter import VolumeConverter
from model.volume.volume import Volume

class VolumeEditTest( BaseCliTest ):
    '''
    Created on Apr 23, 2015
    
    This class attempts to test the volume edit case
    
    @author: nate
    '''
    @patch( "services.volume_service.VolumeService.list_volumes", side_effect=mock_functions.listVolumes)
    @patch( "services.volume_service.VolumeService.get_volume", side_effect=mock_functions.findVolumeById)    
    @patch( "services.volume_service.VolumeService.edit_volume", side_effect=mock_functions.editVolume )
    def test_edit_by_name_by_args(self, mockEdit, mockFindByName, mockList ):
        '''
        Testing a basic volume edit with argument data
        '''
        
        print("Editing a volume by name with arguments")

        args = ["volume", "edit", "-volume_id=MyVol", "-iops_max=13", "-iops_min=2", "-priority=9", "-continuous_protection=300000"]
        
        self.callMessageFormatter(args)
        self.cli.run( args )
        
        assert mockEdit.call_count == 1
        assert mockFindByName.call_count == 1
        assert mockList.call_count == 1
        
        name = mockFindByName.call_args[0][0]
        assert name == "MyVol"
        
        volume = mockEdit.call_args[0][0]
        
        assert volume.qos_policy.iops_min == 2
        assert volume.qos_policy.iops_max == 13
        assert volume.qos_policy.priority == 9
        assert volume.data_protection_policy.commit_log_retention == 300000
        
        print("Edit volume by name was successful.\n\n")

    @patch( "services.volume_service.VolumeService.list_volumes", side_effect=mock_functions.listVolumes)         
    @patch( "services.volume_service.VolumeService.get_volume", side_effect=mock_functions.findVolumeById)
    @patch( "services.volume_service.VolumeService.edit_volume", side_effect=mock_functions.editVolume )
    def test_edit_by_id_by_args(self, mockEdit, mockFindById, mockList ):
        '''
        Testing a basic volume edit by ID with argument data
        '''
        
        print("Editing a volume by ID with arguments")

        args = ["volume", "edit", "-volume_id=5", "-iops_max=100", "-iops_min=2000", "-priority=1", "-continuous_protection=90000"]
        
        self.callMessageFormatter(args)
        self.cli.run( args )
        
        assert mockEdit.call_count == 1
        assert mockFindById.call_count == 1
        assert mockList.call_count == 1
        
        name = mockFindById.call_args[0][0]
        assert name == "5"
        
        volume = mockEdit.call_args[0][0]
        
        assert volume.qos_policy.iops_min == 2000
        assert volume.qos_policy.iops_max == 100
        assert volume.qos_policy.priority == 1
        assert volume.data_protection_policy.commit_log_retention == 90000
        
        print("Edit volume by ID was successful.\n\n")

    @patch( "services.volume_service.VolumeService.list_volumes", side_effect=mock_functions.listVolumes)         
    @patch( "services.volume_service.VolumeService.get_volume", side_effect=mock_functions.findVolumeById)
    @patch( "services.volume_service.VolumeService.edit_volume", side_effect=mock_functions.editVolume )
    def test_edit_by_data(self, mockEdit, mockFindById, mockList ):
        '''
        Testing a basic volume edit with JSON data
        '''            
        
        print("Editing a volume with JSON data")

        volume = Volume()
        volume.qos_policy.iops_min = 100
        volume.qos_policy.iops_max = 2000
        volume.data_protection_policy.commit_log_retention = 100000
        volume.qos_policy.priority = 5
        volume.id = 6
        
        volStr = VolumeConverter.to_json( volume )
        
        args = ["volume", "edit", "-data=" + volStr]
        
        self.callMessageFormatter(args)
        self.cli.run( args )
        
        assert mockEdit.call_count == 1
        assert mockFindById.call_count == 0
        assert mockList.call_count == 1
        
        volume = mockEdit.call_args[0][0]
        
        assert volume.qos_policy.priority == 5
        assert volume.qos_policy.iops_min == 100
        assert volume.qos_policy.iops_max == 2000
        assert volume.data_protection_policy.commit_log_retention == 100000
        
        print("Edit volume by JSON string was successful.\n\n")

    @patch( "services.volume_service.VolumeService.get_volume", side_effect=mock_functions.findVolumeById)        
    @patch( "services.volume_service.VolumeService.edit_volume", side_effect=mock_functions.editVolume )
    def test_edit_by_data_and_id(self, mockEdit, mockGet ):
        '''
        Test that it fails if you include data and an ID or Name
        '''
        
        print("Testing edit volume argument exclusivity")

        args = ["volume", "edit", "-data=SomeString", "-volume_id=5"]
        
        self.callMessageFormatter(args)
        self.cli.run( args )
        
        assert mockEdit.call_count == 0
        
        args[3] = "-volume_name=MyVol"
        
        self.callMessageFormatter(args)
        self.cli.run( args )
        
        assert mockEdit.call_count == 0
        
        print("Edit volume by JSON string and ID successfully failed.\n\n")

    @patch( "services.volume_service.VolumeService.get_volume", side_effect=mock_functions.findVolumeById)        
    @patch( "services.volume_service.VolumeService.edit_volume", side_effect=mock_functions.editVolume) 
    def test_edit_boundaries(self, mockEdit, mockGet):
        '''
        Testing that all the boundary conditions are enforce
        '''
        
        args = ["volume", "edit", "-priority=11", "-iops_min=30", "-iops_max=30", "-continuous_protection=86400", "-volume_id=4"]
        
        self.callMessageFormatter(args)
        
        print("Testing bad priority")

        self.cli.run( args )
        assert mockEdit.call_count == 0
        
        print("Testing bad iops_guarantee")

        args[2] = "-priority=1"
        args[3] = "-iops_min=-1"
        
        self.cli.run( args )
        assert mockEdit.call_count == 0
        
        print("Testing bad iops_limit")

        args[3] = "-iops_min=4000"
        args[4] = "-iops_max=100000"
        
        self.cli.run( args )
        assert mockEdit.call_count == 0
        
        print("Testing bad continuous protection")

        args[4] = "-iops_max=1000"
        args[5] = "-continuous_protection=1000"
        
        self.cli.run( args )
        assert mockEdit.call_count == 0
        
        print("Testing edit boundary conditions passed.\n\n")
