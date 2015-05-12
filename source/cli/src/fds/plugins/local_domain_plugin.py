from abstract_plugin import AbstractPlugin
from fds.services.local_domain_service import LocalDomainService
from fds.services.response_writer import ResponseWriter
from fds.utils.domain_converter import DomainConverter

import json

'''
Created on Apr 13, 2015

A plugin to configure all the parsing for local domain operations and 
to use those arguments to make the appropriate local domain REST calls

@author: nate
'''
class LocalDomainPlugin( AbstractPlugin):
    
    def __init__(self, session):
        AbstractPlugin.__init__(self, session)   
        
        self.__local_domain_service = LocalDomainService(session) 
    
    '''
    @see: AbstractPlugin
    '''
    def build_parser(self, parentParser, session): 
        
        self.__parser = parentParser.add_parser( "local_domain", help="Manage and interact with local domains" )
        self.__subparser = self.__parser.add_subparsers( help="The sub-commands that are available" )
        
        self.create_list_parser( self.__subparser )
        self.create_shutdown_parser( self.__subparser )
        self.create_startup_parser(self.__subparser)
    
    '''
    @see: AbstractPlugin
    '''    
    def detect_shortcut(self, args):
        
        return None
    
    def get_local_domain_service(self):
        return self.__local_domain_service
    
    #Create the various parsers
    
    def create_list_parser(self, subparser):
        '''
        Create the parser for listing the local domains
        '''
        __list_parser = subparser.add_parser( "list", help="Get a list of local domains")
        __list_parser.add_argument( "-" + AbstractPlugin.format_str, help="Specify the format that the result is printed as", choices=["json","tabular"], required=False )
        
        __list_parser.set_defaults( func=self.list_domains, format="tabular" )
        
    def create_shutdown_parser(self, subparser):
        '''
        Create the parser for the shutdown command
        '''
        
        __shutdown_parser = subparser.add_parser( "shutdown", help="Shutdown all nodes and services in a given domain" )
        __shutdown_parser.add_argument( "-" + AbstractPlugin.format_str, help="Specify the format that the result is printed as", choices=["json","tabular"], required=False )        
        __shutdown_parser.add_argument( "-" + AbstractPlugin.domain_name_str, help="The name of the domain you wish to shutdown.", required=True )
        
        __shutdown_parser.set_defaults( func=self.shutdown, format="tabular" )
        
    def create_startup_parser(self, subparser):
        '''
        Create the parser for the startup command
        '''
        
        __startup_parser = subparser.add_parser( "start", help="Start all the nodes and services in a given domain" )
        self.add_format_arg( __startup_parser )
        __startup_parser.add_argument( "-" + AbstractPlugin.domain_name_str, help="The name of the domain you wish to start.", required=True )
        
        __startup_parser.set_defaults( func=self.startup, format="tabular")
    #real work
    
    def list_domains(self, args):
        '''
        do any logic necessary before calling the list domains service call
        '''
        domains = self.get_local_domain_service().get_local_domains()
        
        if ( args[AbstractPlugin.format_str] == "json" ):
            
            j_domains = []
            
            for domain in domains:
                j_domain = DomainConverter.to_json(domain)
                j_domain = json.loads( j_domain )
                j_domains.append( j_domain )
            
            ResponseWriter.writeJson( j_domains )
        else:
            domains = ResponseWriter.prep_domains_for_table( self.session, domains )
            ResponseWriter.writeTabularData( domains )
            
    def shutdown(self, args):
        '''
        shutdown a specific domain
        '''
        response = self.get_local_domain_service().shutdown( args[AbstractPlugin.domain_name_str] )
        
        if response["status"].lower() == "ok":
            self.list_domains(args)
            
    def startup(self, args):
        '''
        start a specific domain gracefully
        '''
        
        response = self.get_local_domain_service().start( args[AbstractPlugin.domain_name_str] )
        
        if response["status"].lower() == "ok":
            self.list_domains(args)
            
            