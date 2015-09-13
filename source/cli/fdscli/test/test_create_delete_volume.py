'''
Created on Apr 9, 2015

@author: nate
'''
import mock_functions
from mock import patch
from base_cli_test import BaseCliTest

class VolumeTest1( BaseCliTest ):
    '''
    This test class handles listing volumes and creating/deleting volumes
    '''
    
    @patch( "services.volume_service.VolumeService.list_volumes", side_effect=mock_functions.listVolumes )
    def test_listVolumes(self, mockService ):
        
        args = ["volume", "list", "-format=json"]
        
        print("Making call: volume list -format=json")

        self.cli.run( args )
         
        self.callMessageFormatter(args)
         
        assert mockService.call_count == 1
        
        print("test_listVolumes passed.\n\n")

    @patch( "services.volume_service.VolumeService.list_volumes", side_effect=mock_functions.listVolumes )
    @patch( "services.volume_service.VolumeService.delete_volume", side_effect=mock_functions.deleteVolume )
    def test_deleteVolume_by_id(self, mockDelete, listCall ):
        
        args = ["volume", "delete", "-volume_id=3" ]
        
        self.callMessageFormatter(args)
        
        self.cli.run( args )
        
        assert mockDelete.call_count == 1
        
        v_id = mockDelete.call_args[0][0]
        
        print("Making sure we call the find method with the ID and get a certain name to the delete call.")

        assert v_id == "3"
        
    @patch( "services.volume_service.VolumeService.list_volumes", side_effect=mock_functions.listVolumes )
    @patch( "services.volume_service.VolumeService.delete_volume", side_effect=mock_functions.deleteVolume )
    def test_deleteVolume_by_name(self, mockDelete, listCall ):
        
        args = ["volume", "delete", "-volume_name=FakeVol" ]
        
        self.callMessageFormatter(args)
        
        self.cli.run( args )
        
        assert mockDelete.call_count == 1
        
        v_id = mockDelete.call_args[0][0]
        
        print("Making sure we call the find method with the name and get a certain ID to the delete call.")

        assert v_id == 1        


    @patch( "services.volume_service.VolumeService.list_volumes", side_effect=mock_functions.listVolumes )
    @patch( "services.volume_service.VolumeService.create_volume", side_effect=mock_functions.createVolume )
    def test_create_with_defaults(self, volumeCreateMethod, listCall):

        args = ["volume", "create", "-name=Franklin"]

        self.callMessageFormatter(args)
      
        self.cli.run( args )
        
        assert volumeCreateMethod.call_count == 1
        
        volume = volumeCreateMethod.call_args[0][0]
        
        print("Checking the call stack to make sure it went to the right place")

        assert volume.data_protection_policy.commit_log_retention == 86400
        assert volume.id == -1
        assert volume.name == "Franklin"
        assert volume.qos_policy.iops_min == 0
        assert volume.qos_policy.iops_max == 0
        assert volume.media_policy == "HYBRID"
        assert volume.qos_policy.priority == 7
        assert volume.settings.type == "OBJECT"
        
        print("test_create_with_defaults passed.\n\n")

    @patch( "services.volume_service.VolumeService.list_volumes", side_effect=mock_functions.listVolumes )
    @patch( "services.volume_service.VolumeService.create_volume", side_effect=mock_functions.createVolume )
    def test_create_with_args(self, volumeCreate, listCall):
         
        args = ["volume", "create", "-name=Franklin2", "-priority=1", "-iops_min=30", "-iops_max=30", "-continuous_protection=86400",
                "-tiering_policy=SSD", "-type=block", "-size=2", "-size_unit=MB"]
         
        self.callMessageFormatter(args)
        self.cli.run( args )
         
        volume = volumeCreate.call_args[0][0]
         
        print("Checking the parameters made it through")

        assert volume.data_protection_policy.commit_log_retention == 86400
        assert volume.id == -1
        assert volume.name == "Franklin2"
        assert volume.qos_policy.iops_min == 30
        assert volume.qos_policy.iops_max == 30
        assert volume.media_policy == "SSD"
        assert volume.qos_policy.priority == 1
        assert volume.settings.type == "BLOCK"
        assert volume.status.current_usage.size == 0
        assert volume.status.current_usage.unit == "GB"   
         
        print("test_create_with_args passed.\n\n")

    @patch( "services.volume_service.VolumeService.list_volumes", side_effect=mock_functions.listVolumes )        
    @patch( "services.volume_service.VolumeService.create_volume", side_effect=mock_functions.createVolume )
    def test_create_boundary_checking(self, volumeCreate, listCall ):
        
        args = ["volume", "create", "-name=Franklin2", "-priority=11", "-iops_min=30", "-iops_max=30", "-continuous_protection=86400",
                "-tiering_policy=SSD", "-type=block", "-size=2", "-size_unit=MB"]
        
        self.callMessageFormatter(args)
        
        print("Testing bad priority")

        self.cli.run( args )
        assert volumeCreate.call_count == 0
        
        print("Testing bad iops_guarantee")

        args[3] = "-priority=1"
        args[4] = "-iops_min=-1"
        
        self.cli.run( args )
        assert volumeCreate.call_count == 0
        
        print("Testing bad iops_limit")

        args[4] = "-iops_min=4000"
        args[5] = "-iops_max=100000"
        
        self.cli.run( args )
        assert volumeCreate.call_count == 0
        
        print("Testing bad continuous protection")

        args[5] = "-iops_max=1000"
        args[6] = "-continuous_protection=1000"
        
        self.cli.run( args )
        assert volumeCreate.call_count == 0
        
        print("Testing bad media policy")

        args[6] = "-continuous_protection=86400"
        args[7] = "-tiering_policy=MY_POLICY"
        
        self.cli.run( args )
        assert volumeCreate.call_count == 0
        
        print("Testing bad volume type")

        args[7] = "-tiering_policy=SSD"
        args[8] = "-type=NFS"
        
        self.cli.run( args )
        assert volumeCreate.call_count == 0
        
        print("Testing bad block size")

        args[8] = "-type=block"
        args[9] = "-size=-1"
        
        self.cli.run( args )
        assert volumeCreate.call_count == 0
        
        print("Testing bad units")

        args[9] = "-size=2"
        args[10] = "-size_unit=EB"
        
        self.cli.run( args )
        assert volumeCreate.call_count == 0
        
        print("test_create_boundary_checking passed.\n\n")
