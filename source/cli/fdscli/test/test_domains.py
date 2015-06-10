from base_cli_test import BaseCliTest
from mock import patch
import mock_functions

class TestDomains(BaseCliTest):
    '''
    Created on Apr 28, 2015
    
    @author: nate
    '''

    @patch( "services.local_domain_service.LocalDomainService.get_local_domains", side_effect=mock_functions.listLocalDomains)
    def test_list_domains(self, mockList):
        '''
        Test that we can list out the domains
        '''
        
        args = ["local_domain", "list"]
        self.callMessageFormatter(args)
        self.cli.run(args)
        
        assert mockList.call_count == 1
        
    @patch( "services.local_domain_service.LocalDomainService.find_domain_by_id", side_effect=mock_functions.findDomainById)
    @patch( "services.local_domain_service.LocalDomainService.shutdown", side_effect=mock_functions.shutdownDomain)
    @patch( "services.local_domain_service.LocalDomainService.get_local_domains", side_effect=mock_functions.listLocalDomains)
    def test_shutdown(self, mockList, mockShutdown, mockFind):
        '''
        Testing that shutdown gets called
        '''
        
        # first test the requirement of the domain name
        args = ["local_domain", "shutdown"]
        
        self.callMessageFormatter(args)
        self.cli.run(args)
        
        assert mockShutdown.call_count == 0
        assert mockList.call_count == 0
        
        args.append( "-domain_id=1" )
        
        self.callMessageFormatter(args)
        self.cli.run(args)
        
        assert mockShutdown.call_count == 1
        assert mockList.call_count == 1
        
        domain = mockShutdown.call_args[0][0]
        
        assert domain.id == "1"
        
    @patch( "services.local_domain_service.LocalDomainService.find_domain_by_id", side_effect=mock_functions.findDomainById) 
    @patch( "services.local_domain_service.LocalDomainService.start", side_effect=mock_functions.startDomain)
    @patch( "services.local_domain_service.LocalDomainService.get_local_domains", side_effect=mock_functions.listLocalDomains)
    def test_startup(self, mockList, mockStart, mockFind):
        '''
        Testing that shutdown gets called
        '''
        
        # first test the requirement of the domain name
        args = ["local_domain", "start"]
        
        self.callMessageFormatter(args)
        self.cli.run(args)
        
        assert mockStart.call_count == 0
        assert mockList.call_count == 0
        
        args.append( "-domain_id=1" )
        
        self.callMessageFormatter(args)
        self.cli.run(args)
        
        assert mockStart.call_count == 1
        assert mockList.call_count == 1
        
        domain = mockStart.call_args[0][0]
        
        assert domain.id == "1"        
