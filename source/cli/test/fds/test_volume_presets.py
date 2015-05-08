from base_cli_test import BaseCliTest
from mock import patch
import mock_functions

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
    
    @patch( "fds.services.volume_service.VolumeService.list_volumes", side_effect=mock_functions.listVolumes)
    @patch( "fds.services.snapshot_policy_service.SnapshotPolicyService.attach_snapshot_policy", side_effect=mock_functions.attachPolicy )
    @patch( "fds.services.snapshot_policy_service.SnapshotPolicyService.create_snapshot_policy", side_effect=mock_functions.createSnapshotPolicy)
    @patch( "fds.services.volume_service.VolumeService.get_timeline_presets", side_effect=mock_functions.listTimelinePresets)
    @patch( "fds.services.volume_service.VolumeService.get_qos_presets", side_effect=mock_functions.listQosPresets)
    @patch( "fds.services.volume_service.VolumeService.find_volume_by_id", side_effect=mock_functions.findVolumeById)
    @patch( "fds.services.volume_service.VolumeService.clone_from_timeline", side_effect=mock_functions.cloneFromTimelineTime)   
    def test_clone_volume(self, mockClone, mockFind, mockQos, mockTimeline, mockCreatePolicy, mockAttachPolicy, mockList):
        '''
        Test cloning a volume with both a timeline preset and a qos preset
        '''
        
        args = ["volume", "clone", "-volume_id=100", "-name=cloned", "-qos_preset_id=1", "-timeline_preset_id=1"]
        self.callMessageFormatter(args)
        self.cli.run(args)
        
        assert mockFind.call_count == 1
        assert mockQos.call_count == 1
        assert mockTimeline.call_count == 1
        assert mockClone.call_count == 1
        assert mockCreatePolicy.call_count == 1
        assert mockAttachPolicy.call_count == 1
        assert mockList.call_count == 1
        
        vol_id = mockFind.call_args[0][0]
        assert vol_id == "100"
        
        volume = mockClone.call_args[0][1]
        assert volume.priority == 1
        assert volume.iops_guarantee == 1
        assert volume.iops_limit == 1
        
        policy = mockCreatePolicy.call_args[0][0]
        
        assert policy.recurrence_rule.frequency == "DAILY"
        assert policy.id == 100
        
        pol_id = mockAttachPolicy.call_args[0][0]
        vol_id = mockAttachPolicy.call_args[0][1]
        
        assert pol_id == 100
        assert vol_id == 354
        
    @patch( "fds.services.volume_service.VolumeService.list_volumes", side_effect=mock_functions.listVolumes)
    @patch( "fds.services.snapshot_policy_service.SnapshotPolicyService.list_snapshot_policies_by_volume", side_effect=mock_functions.listSnapshotPolicies)
    @patch( "fds.services.snapshot_policy_service.SnapshotPolicyService.attach_snapshot_policy", side_effect=mock_functions.attachPolicy )
    @patch( "fds.services.snapshot_policy_service.SnapshotPolicyService.detach_snapshot_policy", side_effect=mock_functions.detachPolicy )
    @patch( "fds.services.snapshot_policy_service.SnapshotPolicyService.create_snapshot_policy", side_effect=mock_functions.createSnapshotPolicy)
    @patch( "fds.services.volume_service.VolumeService.get_timeline_presets", side_effect=mock_functions.listTimelinePresets)
    @patch( "fds.services.volume_service.VolumeService.get_qos_presets", side_effect=mock_functions.listQosPresets)
    @patch( "fds.services.volume_service.VolumeService.find_volume_by_id", side_effect=mock_functions.findVolumeById)
    @patch( "fds.services.volume_service.VolumeService.edit_volume", side_effect=mock_functions.editVolume)           
    def test_edit_volume_with_presets(self, mockEdit, mockFind, mockQos, mockTimeline, mockCreatePolicy, mockDetachPolicy, mockAttachPolicy, mockListSnaps, mockList):
        '''
        Test editing a volume by sending in some preset selections
        '''
        
        args = ["volume", "edit", "-volume_id=100", "-qos_preset_id=1", "-timeline_preset_id=1"]
        self.callMessageFormatter(args)
        self.cli.run(args)
        
        assert mockFind.call_count == 1
        assert mockQos.call_count == 1
        assert mockTimeline.call_count == 1
        assert mockEdit.call_count == 1
        assert mockCreatePolicy.call_count == 1
        assert mockAttachPolicy.call_count == 1
        assert mockList.call_count == 1
        assert mockDetachPolicy.call_count == 1
        assert mockListSnaps.call_count == 1
        
        vol_id = mockFind.call_args[0][0]
        assert vol_id == "100"
        
        volume = mockEdit.call_args[0][0]
        assert volume.priority == 1
        assert volume.iops_guarantee == 1
        assert volume.iops_limit == 1
        
        policy = mockCreatePolicy.call_args[0][0]
        
        assert policy.recurrence_rule.frequency == "DAILY"
        assert policy.id == 100
        
        pol_id = mockAttachPolicy.call_args[0][0]
        vol_id = mockAttachPolicy.call_args[0][1]
        
        assert pol_id == 100
        assert vol_id == "100"
        
        pol_id = mockDetachPolicy.call_args[0][0]
        vol_id = mockDetachPolicy.call_args[0][1]
        
        assert pol_id == 900
        assert vol_id == "100"        
         
        
        
        
        