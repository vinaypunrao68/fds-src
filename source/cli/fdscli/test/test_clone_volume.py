from base_cli_test import BaseCliTest
from mock import patch
import mock_functions
import time
from model.volume import Volume
from utils.volume_converter import VolumeConverter

class VolumeCloneTest( BaseCliTest):
    '''
    Created on Apr 23, 2015
    
    Test out the clone calls
    
    @author: nate
    '''
    
    @patch( "services.volume_service.VolumeService.list_volumes", side_effect=mock_functions.listVolumes )
    @patch( "services.volume_service.VolumeService.find_volume_from_snapshot_id", side_effect=mock_functions.findVolumeBySnapId)
    @patch( "services.volume_service.VolumeService.clone_from_snapshot_id", side_effect=mock_functions.cloneFromSnapshotId)
    def test_clone_from_snapshot_id(self, mockCloneSnap, mockFindBySnap, mockList ):
        '''
        Try to create a clone with a snapshot ID
        '''
        
        print "Trying to create a volume clone by snapshot ID and defaults"
        
        args = ["volume", "clone", "-snapshot_id=35", "-name=CloneVol"]
        
        self.callMessageFormatter(args)
        self.cli.run( args )
        
        assert mockCloneSnap.call_count == 1
        assert mockFindBySnap.call_count == 1
        assert mockList.call_count == 1
        
        find_id = mockFindBySnap.call_args[0][0]
        assert find_id == "35"
        
        snap_id = mockCloneSnap.call_args[0][0]
        volume = mockCloneSnap.call_args[0][1]
        
        assert snap_id == "35"
        assert volume.name == "CloneVol"
        
        print "Clone volume by snapshot ID was successful.\n\n"

    @patch( "services.volume_service.VolumeService.find_volume_by_id", side_effect=mock_functions.findVolumeById)
    @patch( "services.volume_service.VolumeService.find_volume_by_name", side_effect=mock_functions.findVolumeByName)
    @patch( "services.volume_service.VolumeService.list_volumes", side_effect=mock_functions.listVolumes )
    @patch( "services.volume_service.VolumeService.clone_from_timeline", side_effect=mock_functions.cloneFromTimelineTime )
    def test_clone_from_timeline_name(self, mockClone, mockList, mockName, mockId ):
        '''
        Test creating a clone from the timeline with a volume name
        '''
        
        print "Trying to clone a volume from a timeline time and a volume name"
        
        args = ["volume", "clone", "-time=123456789", "-volume_name=MyVol", "-name=ClonedVol"]
        
        self.callMessageFormatter(args)
        self.cli.run( args )
        
        assert mockClone.call_count == 1
        assert mockList.call_count == 1
        assert mockName.call_count == 1
        assert mockId.call_count == 0
        
        name = mockName.call_args[0][0]
        assert name == "MyVol"
        
        a_time = mockClone.call_args[0][0]
        assert a_time == 123456789
        volume = mockClone.call_args[0][1]
        assert volume.name == "ClonedVol"
        
        print "Cloning from timeline time and volume name was successful.\n\n"
        
    @patch( "services.volume_service.VolumeService.find_volume_by_id", side_effect=mock_functions.findVolumeById)
    @patch( "services.volume_service.VolumeService.find_volume_by_name", side_effect=mock_functions.findVolumeByName)
    @patch( "services.volume_service.VolumeService.list_volumes", side_effect=mock_functions.listVolumes )
    @patch( "services.volume_service.VolumeService.clone_from_timeline", side_effect=mock_functions.cloneFromTimelineTime )
    def test_clone_from_timeline_id(self, mockClone, mockList, mockName, mockId ):
        '''
        Test creating a clone from the timeline with a volume ID
        '''
        
        print "Trying to clone a volume from a timeline time and a volume ID"
        
        args = ["volume", "clone", "-time=123456789", "-volume_id=13", "-name=ClonedVol2"]
        
        self.callMessageFormatter(args)
        self.cli.run( args )
        
        assert mockClone.call_count == 1
        assert mockList.call_count == 1
        assert mockName.call_count == 0
        assert mockId.call_count == 1
        
        an_id = mockId.call_args[0][0]
        assert an_id == "13"
        
        a_time = mockClone.call_args[0][0]
        assert a_time == 123456789
        volume = mockClone.call_args[0][1]
        assert volume.name == "ClonedVol2"
        
        print "Cloning from timeline time and volume name was successful.\n\n" 
      
    @patch( "services.volume_service.VolumeService.find_volume_by_name", side_effect=mock_functions.findVolumeByName)
    @patch( "services.volume_service.VolumeService.list_volumes", side_effect=mock_functions.listVolumes )
    @patch( "services.volume_service.VolumeService.clone_from_timeline", side_effect=mock_functions.cloneFromTimelineTime )        
    def test_clone_from_args(self, mockClone, mockList, mockName ):
        '''
        Test to see if new QoS items are passed through from the arg list
        '''
        
        print "Trying to create a clone with different QoS settings"
        
        args = ["volume", "clone", "-volume_name=FirstVol", "-name=ClonedVol", "-priority=9", "-iops_guarantee=5000", "-iops_limit=3000", "-continuous_protection=86500"]
        
        now = int(time.time())
        
        self.callMessageFormatter(args)
        self.cli.run( args )
        
        assert mockClone.call_count == 1
        assert mockList.call_count == 1
        assert mockName.call_count == 1
        
        a_time = mockClone.call_args[0][0]
        volume = mockClone.call_args[0][1]
               
        assert volume.priority == 9
        assert volume.iops_guarantee == 5000
        assert volume.iops_limit == 3000
        assert volume.continuous_protection == 86500
        assert a_time >= now
        
        print "Cloning volume with new QoS settings from args was successful.\n\n"
        
    @patch( "services.volume_service.VolumeService.find_volume_by_name", side_effect=mock_functions.findVolumeByName)
    @patch( "services.volume_service.VolumeService.list_volumes", side_effect=mock_functions.listVolumes )
    @patch( "services.volume_service.VolumeService.clone_from_timeline", side_effect=mock_functions.cloneFromTimelineTime )        
    def test_clone_from_data(self, mockClone, mockList, mockName ):
        '''
        Test to see if new QoS settings are accepted from a JSON data string
        '''
        
        print "Trying to clone a volume with different QoS settings from JSON string"
        
        now = int(time.time())
        
        volume = Volume()
        volume.iops_guarantee = 30000
        volume.iops_limit = 100500
        volume.priority = 1
        volume.continuous_protection = 90000
        
        volStr = VolumeConverter.to_json( volume )
        
        args = ["volume", "clone", "-volume_name=MyVol", "-data=" + volStr, "-name=ClonedVol7"]
        self.callMessageFormatter(args)
        self.cli.run( args )
        
        assert mockClone.call_count == 1
        assert mockName.call_count == 1
        assert mockList.call_count == 1
        
        a_time = mockClone.call_args[0][0]
        volume = mockClone.call_args[0][1]
        
        assert a_time >= now
        assert volume.iops_guarantee == 30000
        assert volume.iops_limit == 100500
        assert volume.priority == 1
        assert volume.continuous_protection == 90000
        
        print "Cloning volume with new QoS setting from a JSON string was successful."
        
    @patch( "services.volume_service.VolumeService.clone_from_timeline", side_effect=mock_functions.cloneFromTimelineTime )
    @patch( "services.volume_service.VolumeService.clone_from_snapshot_id", side_effect=mock_functions.cloneFromSnapshotId)
    def test_boundary_conditions(self, mockSnap, mockTime):
        '''
        Test the expected failure cases if arguments are out of bounds
        '''
        
        print "Testing boundary conditions on the arguments"
        
        args = ["volume", "clone", "-priority=9", "-iops_guarantee=1000", "-iops_limit=3000", "-continuous_protection=86700", "-name=NewVol"]
        
        print "Testing need for volume_id, snapshot_id or volume_name"
        self.callMessageFormatter(args)
        self.cli.run( args )
        assert mockSnap.call_count == 0
        assert mockTime.call_count == 0
        
        print "Testing that only one can be specified"
        args.append( "-volume_name=SomeVol")
        args.append( "-volume_id=35")
        self.callMessageFormatter(args)
        self.cli.run( args )
        assert mockSnap.call_count == 0
        assert mockTime.call_count == 0
        
        print "Testing the name requirement"
        args.remove( args[8])
        args.remove( args[6])
        self.callMessageFormatter(args)
        self.cli.run( args )
        assert mockSnap.call_count == 0
        assert mockTime.call_count == 0
                
        print "Testing bad priority"  
        args[2] = "-priority=11"      
        self.callMessageFormatter(args)
        self.cli.run( args )
        assert mockSnap.call_count == 0
        assert mockTime.call_count == 0
        
        print "Testing bad iops_guarantee"
        args[2] = "-priority=1"
        args[3] = "-iops_guarantee=-1"
        self.callMessageFormatter(args)
        self.cli.run( args )
        assert mockSnap.call_count == 0
        assert mockTime.call_count == 0
        
        print "Testing bad iops_limit"
        args[3] = "-iops_guarantee=4000"
        args[4] = "-iops_limit=100000"
        self.callMessageFormatter(args)
        self.cli.run( args )
        assert mockSnap.call_count == 0
        assert mockTime.call_count == 0
        
        print "Testing bad continuous protection"
        args[4] = "-iops_limit=1000"
        args[5] = "-continuous_protection=1000"
        self.callMessageFormatter(args)
        self.cli.run( args )
        assert mockSnap.call_count == 0
        assert mockTime.call_count == 0
