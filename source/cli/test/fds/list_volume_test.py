'''
Created on Apr 9, 2015

@author: nate
'''

import unittest
from fds import fdscli
from mock import patch
from mock_auth import MockFdsAuth
from fds.services.rest_helper import RESTHelper
from mock_volume_service import MockVolumeService

class ListVolumeTest(unittest.TestCase):
    
    @patch( "fds.services.volume_service.VolumeService", new_callable=MockVolumeService)
    def test(self, mockService ):
        token = MockFdsAuth().login()
        rh = RESTHelper( token )
        
        args = ["volume", "list", "-format=json"]
        
        cli = fdscli.FDSShell(rh)
        cli.run( args )
        
        print 'Out: ' + token

if __name__ == '__main__':
    unittest.main()

