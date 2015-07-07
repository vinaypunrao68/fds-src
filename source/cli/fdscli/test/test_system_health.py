from mock import patch
import mock_functions
from base_cli_test import BaseCliTest

class TestSystemHealth(BaseCliTest):
    '''
    Created on Jun 30, 2015
    
    @author: nate
    '''

    @patch( "services.stats_service.StatsService.get_system_health_report", side_effect=mock_functions.getSystemHealth )
    def test_system_health(self, health):
        
        args = ["system_health"]
        
        self.callMessageFormatter(args)
        self.cli.run(args)
        
        assert health.call_count == 1