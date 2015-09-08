from mock import patch
import mock_functions
from base_cli_test import BaseCliTest

class SnapshotTest(BaseCliTest):
    '''
    Created on Apr 22, 2015
    
    This class tests out the snapshot functionality with regards to the volumes
    
    @author: nate
    '''
        
    @patch( "services.volume_service.VolumeService.get_volume", side_effect=mock_functions.findVolumeById )        
    @patch( "services.volume_service.VolumeService.list_snapshots", side_effect=mock_functions.listSnapshots )
    def test_list_snapshots_by_id(self, mockList, mockFind ):
        '''
        list the snapshots for a given volume
        '''
        
        print("Trying to list snapshots for a volume")

        args = ["volume", "list_snapshots", "-volume_id=100"]
        
        self.callMessageFormatter(args)
        self.cli.run( args )
        
        volId = mockList.call_args[0][0]
        
        assert mockList.call_count == 1
        assert mockFind.call_count == 0
        
        assert volId == "100"
        
        print("List snapshots by volume ID was successful.")

    @patch( "services.volume_service.VolumeService.list_snapshots", side_effect=mock_functions.listSnapshots )        
    def test_list_snapshot_failure(self, mockList):
        '''
        Make sure we don't call the list call if the arguments for volume were not provided
        '''
        
        print("Trying an expected failure case")

        args = ["volume", "list_snapshots"]
        
        self.callMessageFormatter(args)
        self.cli.run( args )
        
        assert mockList.call_count == 0
        
        print("Failed successfully.")

    @patch( "services.volume_service.VolumeService.get_volume", side_effect=mock_functions.findVolumeById )
    @patch( "services.volume_service.VolumeService.list_snapshots", side_effect=mock_functions.listSnapshots )
    @patch( "services.volume_service.VolumeService.create_snapshot", side_effect=mock_functions.createSnapshot)
    def test_create_snapshots(self, mockCreate, mockList, mockFindId ):
        '''
        basic snapshot creation
        '''
        print("Trying to create a snapshot from a volume by volume name")

        args = ["volume", "create_snapshot", "-name=MySnap", "-volume_id=100"]
        
        self.callMessageFormatter(args)
        
        self.cli.run( args )
        
        assert mockCreate.call_count == 1
        assert mockList.call_count == 1
        assert mockFindId.call_count == 1
        
        snapshot = mockCreate.call_args[0][0]
        
        assert snapshot.volume_id == "100"
        assert snapshot.name == "MySnap"
        
        print("Snapshot created successfully.")

    @patch( "services.volume_service.VolumeService.get_volume", side_effect=mock_functions.findVolumeById )
    @patch( "services.volume_service.VolumeService.list_snapshots", side_effect=mock_functions.listSnapshots )
    @patch( "services.volume_service.VolumeService.create_snapshot", side_effect=mock_functions.createSnapshot)
    def test_create_snapshots_by_id(self, mockCreate, mockList, mockFind ):
        '''
        snapshot creation with a volume ID instead of a name
        '''
        print("Trying to create a snapshot by using the volume ID")

        args = ["volume", "create_snapshot", "-name=MySnap", "-volume_id=5"]
        
        self.callMessageFormatter(args)
        self.cli.run( args )
        
        assert mockCreate.call_count == 1
        assert mockList.call_count == 1
        assert mockFind.call_count == 1
        
        snapshot = mockCreate.call_args[0][0]
        idSearch = mockFind.call_args[0][0]
        
        assert idSearch == "5"
        assert snapshot.name == "MySnap"
        assert snapshot.volume_id == "5"    
        
        print("Snapshot created successfully.")

    @patch( "services.volume_service.VolumeService.list_snapshots", side_effect=mock_functions.listSnapshots )
    def test_create_snapshot_failure(self, mockList ):
        '''
        try to create a snapshot without giving any volume information
        '''
        print("Testing expected failure when no volume information is provided.")

        args = ["volume", "create_snapshot", "-name=MySnap"]
        
        self.callMessageFormatter(args)
        self.cli.run( args )
        
        assert mockList.call_count == 0
        
        print("Failed correctly.")
