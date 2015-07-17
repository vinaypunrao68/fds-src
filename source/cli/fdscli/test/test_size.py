from base_cli_test import BaseCliTest
from model.common.size import Size

class TestSize(BaseCliTest):
    '''
    Created on Jul 16, 2015
    
    @author: nate
    '''

    def test_bytes_conversion(self):
        
        size = Size( size=8, unit="KB" )
        assert size.get_bytes() == (1024*8)
        
        size = Size( size=108, unit="MB" )
        assert size.get_bytes() == (108*pow(1024,2))
        
        size = Size( size=11, unit="GB" )
        assert size.get_bytes() == (11*pow(1024,3))