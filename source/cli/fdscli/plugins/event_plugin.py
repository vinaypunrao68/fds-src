from . import abstract_plugin

'''
Created on Apr 13, 2015

A plugin to setup the parsing for the event gathering arguments and
to enact the correct calls to retrieve the events

@author: nate
'''
from services.fds_auth import FdsAuth
class EventPlugin( abstract_plugin.AbstractPlugin):
    
    def __init__(self):
        abstract_plugin.AbstractPlugin.__init__(self)
        
    '''
    @see: AbstractPlugin
    '''
    def build_parser(self, parentParser, session): 
        
        self.session = session
        
        if not session.is_allowed( FdsAuth.VOL_MGMT ):
            return
        
        self.__parser = parentParser.add_parser( "event", help="Gather system events" )
        self.__subparser = self.__parser.add_subparsers( help="The sub-commands that are available")
     
    '''
    @see: AbstractPlugin
    '''   
    def detect_shortcut(self, args):
        
        return None