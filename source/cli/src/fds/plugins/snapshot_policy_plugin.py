import abstract_plugin

'''
Created on Apr 13, 2015

A plugin to handle all the parsing for snapshot policy management
and to route those options to the appropriate snapshot policy 
management calls

@author: nate
'''
class SnapshotPolicyPlugin( abstract_plugin.AbstractPlugin):
    
    def __init__(self, session):
        abstract_plugin.AbstractPlugin.__init__(self, session)    
    
    '''
    @see: AbstractPlugin
    '''
    def build_parser(self, parentParser, session): 
        
        self.__parser = parentParser.add_parser( "snapshot_policy", help="Create, edit, delete and manipulate snapshot policies" )
        self.__subparser = self.__parser.add_subparsers( help="The sub-commands that are available")
     
    '''
    @see: AbstractPlugin
    '''   
    def detect_shortcut(self, args):
        
        return None