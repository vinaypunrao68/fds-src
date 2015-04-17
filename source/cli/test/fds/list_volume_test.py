'''
Created on Apr 9, 2015

@author: nate
'''

import unittest
from fds import fdscli
from mock import patch
from mock_auth import MockFdsAuth

class ListVolumeTest(unittest.TestCase):
    
    @patch( "fds.services.volume_service.VolumeService.listVolumes", return_value="Done")
    def test(self, mockService ):
        token = MockFdsAuth().login()
        
        args = ["volume", "list", "-format=json"]
        
        cli = fdscli.FDSShell(token)
        cli.run( args )
        
        assert mockService.assert_called()

if __name__ == '__main__':
    unittest.main()

