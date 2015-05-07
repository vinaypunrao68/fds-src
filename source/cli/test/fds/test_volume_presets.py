from base_cli_test import BaseCliTest
from mock import patch
import mock_functions
from fds.utils.preset_converter import PresetConverter

class TestVolumePresets(BaseCliTest):
    '''
    Created on May 7, 2015
    
    @author: nate
    '''
    @patch( "fds.services.volume_service.VolumeService.get_qos_presets", side_effect=mock_functions.listQosPresets)
    @patch( "fds.services.volume_service.VolumeService.get_timeline_presets", side_effect=mock_functions.listTimelinePresets)
    def test_list_presets(self, mockTime, mockQos):
        '''
        Test whether our preset listing works
        '''
        
        args = ["presets","list"]
        self.callMessageFormatter(args)
        self.cli.run(args)
        
        assert mockTime.call_count == 1
        assert mockQos.call_count == 1
        
        args = ["presets","list", "-type=timeline"]
        self.callMessageFormatter(args)
        self.cli.run(args)
        
        assert mockTime.call_count == 2
        assert mockQos.call_count == 1
        
        args = ["presets","list", "-type=qos"]
        self.callMessageFormatter(args)
        self.cli.run(args)
        
        assert mockTime.call_count == 2
        assert mockQos.call_count == 2   
        
    @patch( "fds.services.volume_service.VolumeService.list_volumes", side_effect=mock_functions.listVolumes)
    @patch( "fds.services.volume_service.VolumeService.get_qos_presets", side_effect=mock_functions.listQosPresets)
    @patch( "fds.services.volume_service.VolumeService.create_volume", side_effect=mock_functions.createVolume)
    def test_create_volume_with_qos(self, mockCreate, mockQos, mockList):     
        '''
        Try to create a volume using a qos preset
        '''
        
        args = ["volume","create","-name=Test", "-qos_preset_id=1"]
        self.callMessageFormatter(args)
        self.cli.run(args)
        
        assert mockCreate.call_count == 1
        assert mockQos.call_count == 1
        
        volume = mockCreate.call_args[0][0]
        preset_id = mockQos.call_args[1]["preset_id"]
        
        assert preset_id == "1"
        
        assert volume.priority == 1
        assert volume.iops_guarantee == 1
        assert volume.iops_limit == 1
        
        
        
        
        
        
        
         
        
        
        
        