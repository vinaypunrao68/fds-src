import unittest
from mock import patch
from mock_auth import MockFdsAuth
from fds import fdscli
import mock_functions

class SnapshotTest(unittest.TestCase):
    '''
    Created on Apr 22, 2015
    
    @author: nate
    '''

    @classmethod
    def setUpClass(self):
        print "Setting up the test..."
        
        auth = MockFdsAuth()
        auth.login()
        self.__cli = fdscli.FDSShell( auth ) 
        print "Done with setup\n\n"
        
    def callMessageFormatter(self, args):
        
        message = ""
        
        for arg in args:
            message += arg + " "
            
        print "Making call: " + message
        
    @patch( "fds.services.volume_service.VolumeService.find_volume_by_name", side_effect=mock_functions.findVolumeByName )        
    @patch( "fds.services.volume_service.VolumeService.list_snapshots", side_effect=mock_functions.listSnapshots )
    def test_list_snapshots_by_id(self, mockList, mockFind ):
        '''
        list the snapshots for a given volume
        '''
        
        print "Trying to list snapshots for a volume"
        
        args = ["volume", "list_snapshots", "-volume_id=100"]
        
        self.callMessageFormatter(args)
        self.__cli.run( args )
        
        volId = mockList.call_args[0][0]
        
        assert mockList.call_count == 1
        assert mockFind.call_count == 0
        
        assert volId == "100"
        
        print "List snapshots by volume ID was successful."   
        
    @patch( "fds.services.volume_service.VolumeService.find_volume_by_name", side_effect=mock_functions.findVolumeByName )        
    @patch( "fds.services.volume_service.VolumeService.list_snapshots", side_effect=mock_functions.listSnapshots )
    def test_list_snapshots_by_name(self, mockList, mockFind ):
        '''
        list the snapshots for a given volume
        '''
        
        print "Trying to list snapshots for a volume"
        
        args = ["volume", "list_snapshots", "-volume_name=MyVol"]
        
        self.callMessageFormatter(args)
        self.__cli.run( args )
        
        volId = mockList.call_args[0][0]
        name = mockFind.call_args[0][0]
        
        assert mockList.call_count == 1
        assert mockFind.call_count == 1
        
        assert volId == 100
        assert name == "MyVol"
        
        print "List snapshots by volume name was successful."
        
    @patch( "fds.services.volume_service.VolumeService.list_snapshots", side_effect=mock_functions.listSnapshots )        
    def test_list_snapshot_failure(self, mockList):
        '''
        Make sure we don't call the list call if the arguments for volume were not provided
        '''
        
        print "Trying an expected failure case"
        
        args = ["volume", "list_snapshots"]
        
        self.callMessageFormatter(args)
        self.__cli.run( args )
        
        assert mockList.call_count == 0
        
        print "Failed successfully."
    
    @patch( "fds.services.volume_service.VolumeService.find_volume_by_name", side_effect=mock_functions.findVolumeByName )    
    @patch( "fds.services.volume_service.VolumeService.find_volume_by_id", side_effect=mock_functions.findVolumeById )
    @patch( "fds.services.volume_service.VolumeService.list_snapshots", side_effect=mock_functions.listSnapshots )
    @patch( "fds.services.volume_service.VolumeService.create_snapshot", side_effect=mock_functions.createSnapshot)
    def test_create_snapshots(self, mockCreate, mockList, mockFindId, mockFindName ):
        '''
        basic snapshot creation
        '''
        print "Trying to create a snapshot from a volume by volume name"
        
        args = ["volume", "create_snapshot", "-name=MySnap", "-volume_name=Mine"]
        
        self.callMessageFormatter(args)
        
        self.__cli.run( args )
        
        assert mockCreate.call_count == 1
        assert mockList.call_count == 1
        assert mockFindId.call_count == 0
        assert mockFindName.call_count == 2
        
        snapshot = mockCreate.call_args[0][0]
        name = mockFindName.call_args[0][0]
        
        assert name == "Mine"
        assert snapshot.volume_id == 100
        assert snapshot.name == "MySnap"
        
        print "Snapshot created successfully."
        
    @patch( "fds.services.volume_service.VolumeService.find_volume_by_id", side_effect=mock_functions.findVolumeById )
    @patch( "fds.services.volume_service.VolumeService.list_snapshots", side_effect=mock_functions.listSnapshots )
    @patch( "fds.services.volume_service.VolumeService.create_snapshot", side_effect=mock_functions.createSnapshot)
    def test_create_snapshots_by_id(self, mockCreate, mockList, mockFind ):
        '''
        snapshot creation with a volume ID instead of a name
        '''
        print "Trying to create a snapshot by using the volume ID"
        
        args = ["volume", "create_snapshot", "-name=MySnap", "-volume_id=5"]
        
        self.callMessageFormatter(args)
        self.__cli.run( args )
        
        assert mockCreate.call_count == 1
        assert mockList.call_count == 1
        assert mockFind.call_count == 1
        
        snapshot = mockCreate.call_args[0][0]
        idSearch = mockFind.call_args[0][0]
        
        assert idSearch == "5"
        assert snapshot.name == "MySnap"
        assert snapshot.volume_id == "5"    
        
        print "Snapshot created successfully."  
        
    @patch( "fds.services.volume_service.VolumeService.list_snapshots", side_effect=mock_functions.listSnapshots )
    def test_create_snapshot_failure(self, mockList ):
        '''
        try to create a snapshot without giving any volume information
        '''
        print "Testing expected failure when no volume information is provided."
        
        args = ["volume", "create_snapshot", "-name=MySnap"]
        
        self.callMessageFormatter(args)
        self.__cli.run( args )
        
        assert mockList.call_count == 0
        
        print "Failed correctly."          
        
if __name__ == '__main__':
    unittest.main()        
        