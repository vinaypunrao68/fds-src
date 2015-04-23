'''
Created on Apr 9, 2015

@author: nate
'''

import unittest
import mock_functions
from fds import fdscli
from mock import patch
from mock_auth import MockFdsAuth

class VolumeTest1(unittest.TestCase):
    '''
    This test class handles listing volumes and creating/deleting volumes
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
    
    @patch( "fds.services.volume_service.VolumeService.list_volumes", side_effect=mock_functions.listVolumes )
    def test_listVolumes(self, mockService ):
        
        args = ["volume", "list", "-format=json"]
        
        print "Making call: volume list -format=json"

        self.__cli.run( args )
         
        self.callMessageFormatter(args)
         
        assert mockService.call_count == 1
        
        print "test_listVolumes passed.\n\n"
        
    @patch( "fds.services.volume_service.VolumeService.list_volumes", side_effect=mock_functions.listVolumes )
    @patch( "fds.services.volume_service.VolumeService.delete_volume", side_effect=mock_functions.deleteVolume )
    def test_deleteVolume(self, mockCall, listCall ):
        
        args = ["volume", "delete", "-volume_name=NewOne"]
        
        self.callMessageFormatter(args)
        
        self.__cli.run( args )
        
        assert mockCall.call_count == 1
        
        name = mockCall.call_args[0][0]
        
        assert name == "NewOne"
        
    @patch( "fds.services.volume_service.VolumeService.list_volumes", side_effect=mock_functions.listVolumes )
    @patch( "fds.services.volume_service.VolumeService.delete_volume", side_effect=mock_functions.deleteVolume )
    @patch( "fds.services.volume_service.VolumeService.find_volume_by_id", side_effect=mock_functions.findVolumeById )
    def test_deleteVolume_by_id(self, mockFind, mockDelete, listCall ):
        
        args = ["volume", "delete", "-volume_id=3" ]
        
        self.callMessageFormatter(args)
        
        self.__cli.run( args )
        
        assert mockDelete.call_count == 1
        assert mockFind.call_count == 1
        
        name = mockDelete.call_args[0][0]
        an_id = mockFind.call_args[0][0]
        
        print "Making sure we call the find method with the ID and get a certain name to the delete call."
        
        assert name == "VolumeName"
        assert an_id == "3"

    @patch( "fds.services.volume_service.VolumeService.list_volumes", side_effect=mock_functions.listVolumes )
    @patch( "fds.services.volume_service.VolumeService.create_volume", side_effect=mock_functions.createVolume )
    def test_create_with_defaults(self, volumeCreateMethod, listCall):

        args = ["volume", "create", "-name=Franklin"]

        self.callMessageFormatter(args)
      
        self.__cli.run( args )
        
        assert volumeCreateMethod.call_count == 1
        
        volume = volumeCreateMethod.call_args[0][0]
        
        print "Checking the call stack to make sure it went to the right place"
        
        assert volume.continuous_protection == 86400
        assert volume.id == -1
        assert volume.name == "Franklin"
        assert volume.iops_guarantee == 0
        assert volume.iops_limit == 0
        assert volume.media_policy == "HDD_ONLY"
        assert volume.priority == 7
        assert volume.type == "object"
        
        print "test_create_with_defaults passed.\n\n"

    @patch( "fds.services.volume_service.VolumeService.list_volumes", side_effect=mock_functions.listVolumes )
    @patch( "fds.services.volume_service.VolumeService.create_volume", side_effect=mock_functions.createVolume )
    def test_create_with_args(self, volumeCreate, listCall):
         
        args = ["volume", "create", "-name=Franklin2", "-priority=1", "-iops_guarantee=30", "-iops_limit=30", "-continuous_protection=86400",
                "-media_policy=SSD_ONLY", "-type=block", "-size=2", "-size_unit=MB"]
         
        self.callMessageFormatter(args)
        self.__cli.run( args )
         
        volume = volumeCreate.call_args[0][0]
         
        print "Checking the parameters made it through"
         
        assert volume.continuous_protection == 86400
        assert volume.id == -1
        assert volume.name == "Franklin2"
        assert volume.iops_guarantee == 30
        assert volume.iops_limit == 30
        assert volume.media_policy == "SSD_ONLY"
        assert volume.priority == 1
        assert volume.type == "block"
        assert volume.current_size == 2
        assert volume.current_units == "MB"   
         
        print "test_create_with_args passed.\n\n"  
        
    @patch( "fds.services.volume_service.VolumeService.list_volumes", side_effect=mock_functions.listVolumes )        
    @patch( "fds.services.volume_service.VolumeService.create_volume", side_effect=mock_functions.createVolume )
    def test_create_boundary_checking(self, volumeCreate, listCall ):
        
        args = ["volume", "create", "-name=Franklin2", "-priority=11", "-iops_guarantee=30", "-iops_limit=30", "-continuous_protection=86400",
                "-media_policy=SSD_ONLY", "-type=block", "-size=2", "-size_unit=MB"]
        
        self.callMessageFormatter(args)
        
        print "Testing bad priority"        
        self.__cli.run( args )
        assert volumeCreate.call_count == 0
        
        print "Testing bad iops_guarantee"
        args[3] = "-priority=1"
        args[4] = "-iops_guarantee=-1"
        
        self.__cli.run( args )
        assert volumeCreate.call_count == 0
        
        print "Testing bad iops_limit"
        args[4] = "-iops_guarantee=4000"
        args[5] = "-iops_limit=100000"
        
        self.__cli.run( args )
        assert volumeCreate.call_count == 0
        
        print "Testing bad continuous protection"
        args[5] = "-iops_limit=1000"
        args[6] = "-continuous_protection=1000"
        
        self.__cli.run( args )
        assert volumeCreate.call_count == 0
        
        print "Testing bad media policy"
        args[6] = "-continuous_protection=86400"
        args[7] = "-media_policy=MY_POLICY"
        
        self.__cli.run( args )
        assert volumeCreate.call_count == 0
        
        print "Testing bad volume type"
        args[7] = "-media_policy=SSD_ONLY"
        args[8] = "-type=NFS"
        
        self.__cli.run( args )
        assert volumeCreate.call_count == 0
        
        print "Testing bad block size"
        args[8] = "-type=block"
        args[9] = "-size=1025"
        
        self.__cli.run( args )
        assert volumeCreate.call_count == 0
        
        print "Testing bad units"
        args[9] = "-size=2"
        args[10] = "-size_unit=EB"
        
        self.__cli.run( args )
        assert volumeCreate.call_count == 0
        
        print "test_create_boundary_checking passed.\n\n"

if __name__ == '__main__':
    unittest.main()

